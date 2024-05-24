#include <render/direction_light.h>
#include <render/material.h>
#include <render/mesh.h>
#include <render/scene.h>
#include "camera.h"
#include <application.h>
#include <render/debug_arrow.h>
#include <render/debug_bones.h>
#include <new_time.h>
#include <imgui/imgui.h>
#include "ImGuizmo.h"
#include "Filter.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/animation/offline/animation_optimizer.h"

struct UserCamera
{
  glm::mat4 transform;
  mat4x4 projection;
  ArcballCamera arcballCamera;
};

struct Character
{
  glm::mat4 transform;
  MeshPtr mesh;
  MaterialPtr material;

  SkeletonPtr skeleton_;
  std::shared_ptr<ozz::animation::SamplingJob::Context> context_;

  // Buffer of local transforms as sampled from animation_.
  std::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  std::vector<ozz::math::Float4x4> models_;

  AnimationPtr currentAnimation;
  float animTime = 0;
};

struct Scene
{
  DirectionLight light;

  UserCamera userCamera;

  std::vector<Character> characters;

};

static std::unique_ptr<Scene> scene;
static std::vector<std::string> animationList;


static struct {
  bool show_bones = true;
  bool show_arrows = true;
  bool show_mesh = true;
} visualisation_params;

static struct {
  float tolerance;
  float distance;
  float speed = 1;
  bool pause = false;
  bool loop = true;
  bool optimized = false;
} animation_params;

static AnimationInfo animation_info;

#include <filesystem>
static std::vector<std::string> scan_animations(const char *path)
{
  std::vector<std::string> animations;
  animations.push_back("None");
  for (auto &p : std::filesystem::recursive_directory_iterator(path))
  {
    auto filePath = p.path();
    if (p.is_regular_file() && filePath.extension() == ".fbx")
      animations.push_back(filePath.string());
  }
  return animations;
}

void game_init()
{
  animationList = scan_animations("resources/Animations");
  scene = std::make_unique<Scene>();
  scene->light.lightDirection = glm::normalize(glm::vec3(-1, -1, 0));
  scene->light.lightColor = glm::vec3(1.f);
  scene->light.ambient = glm::vec3(0.2f);

  scene->userCamera.projection = glm::perspective(90.f * DegToRad, get_aspect_ratio(), 0.01f, 500.f);

  ArcballCamera &cam = scene->userCamera.arcballCamera;
  cam.curZoom = cam.targetZoom = 0.5f;
  cam.maxdistance = 5.f;
  cam.distance = cam.curZoom * cam.maxdistance;
  cam.lerpStrength = 10.f;
  cam.mouseSensitivity = 0.5f;
  cam.wheelSensitivity = 0.05f;
  cam.targetPosition = glm::vec3(0.f, 1.f, 0.f);
  cam.targetRotation = cam.curRotation = glm::vec2(DegToRad * -90.f, DegToRad * -30.f);
  cam.rotationEnable = false;

  scene->userCamera.transform = calculate_transform(scene->userCamera.arcballCamera);

  input.onMouseButtonEvent += [](const SDL_MouseButtonEvent &e) { arccam_mouse_click_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseMotionEvent += [](const SDL_MouseMotionEvent &e) { arccam_mouse_move_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseWheelEvent += [](const SDL_MouseWheelEvent &e) { arccam_mouse_wheel_handler(e, scene->userCamera.arcballCamera); };


  auto material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
  std::fflush(stdout);
  material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));
  
  SceneAsset sceneAsset = load_scene("resources/MotusMan_v55/MotusMan_v55.fbx", SceneAsset::LoadScene::Meshes | SceneAsset::LoadScene::Skeleton);
  Character &character = scene->characters.emplace_back(Character{
    glm::identity<glm::mat4>(),
    sceneAsset.meshes[0],
    std::move(material),
    sceneAsset.skeleton});

  const int num_soa_joints = character.skeleton_->num_soa_joints();
  character.locals_.resize(num_soa_joints);
  const int num_joints = character.skeleton_->num_joints();
  character.models_.resize(num_joints);

  // Allocates a context that matches animation requirements.
  character.context_ = std::make_shared<ozz::animation::SamplingJob::Context>(num_joints);

  create_arrow_render();
  create_bone_render();
  std::fflush(stdout);
}

void render_imguizmo(ImGuizmo::OPERATION &mCurrentGizmoOperation, ImGuizmo::MODE &mCurrentGizmoMode)
{
  if (ImGui::Begin("gizmo window"))
  {
    if (ImGui::IsKeyPressed('Z'))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed('E'))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed('R')) // r Key
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
  }
  ImGui::End();
}

void imgui_render()
{
  ImGuizmo::BeginFrame();
  for (Character &character : scene->characters)
  {
    // character.skeleton.updateLocalTransforms();
    // const RuntimeSkeleton &skeleton = character.skeleton;
    // size_t nodeCount = skeleton.ref->nodeCount;
    // static size_t idx = 0;
    // if (ImGui::Begin("Skeleton view"))
    // {
    //   for (size_t i = 0; i < nodeCount; i++)
    //   {
    //     ImGui::Text("%d) %s", int(i), skeleton.ref->names[i].c_str());
    //     ImGui::SameLine();
    //     ImGui::PushID(i);
    //     if (ImGui::Button("edit"))
    //     {
    //       idx = i;
    //     }
    //     ImGui::PopID();
    //   }
    // }
    // const auto &skeleton = *character.skeleton_;
    // size_t nodeCount = skeleton.num_joints();

    // if (ImGui::Begin("Skeleton view"))
    // {
    //   for (size_t i = 0; i < nodeCount; i++)
    //   {
    //     ImGui::Text("%d) %s", int(i), skeleton.joint_names()[i]);
    //   }
    // }
    // ImGui::End();

    if (ImGui::Begin("Visualisition"))
    {
      ImGui::Checkbox("Show mesh", &visualisation_params.show_mesh);
      ImGui::Checkbox("Show bones", &visualisation_params.show_bones);
      ImGui::Checkbox("Show arrows", &visualisation_params.show_arrows);
    }
    ImGui::End();

    static int item = 0;
    if (ImGui::Begin("Animation list")) {
      if (ImGui::ComboWithFilter("##anim", &item, animationList)) {
        AnimationPtr animation;
        if (item > 0)
        {
          SceneAsset sceneAsset;
          sceneAsset = load_scene(animationList[item].c_str(),
                                            SceneAsset::LoadScene::Skeleton | SceneAsset::LoadScene::Animation,
                                            0.f, 0.f, &animation_info);
          if (!sceneAsset.animations.empty()) {
            animation = sceneAsset.animations[0];
          }
          animation_params.optimized = false;
        }
        character.currentAnimation = animation;
        character.animTime = 0;
      }
      ImGui::Checkbox("Pause", &animation_params.pause);
      ImGui::Checkbox("Loop", &animation_params.loop);
      float finishTime =  character.currentAnimation ?  character.currentAnimation->duration() : 0;
      ImGui::SliderFloat("Animation time", &character.animTime, 0, finishTime);
      ImGui::SliderFloat("Speed", &animation_params.speed, 0, 3);
      if (ImGui::Button("Reset speed")) {
        animation_params.speed = 1.0f;
      }
      ImGui::SliderFloat("Tolerance (mm)", &animation_params.tolerance, 0, 100);
      ImGui::SliderFloat("Distance (mm)", &animation_params.distance, 0, 1000);
      if (ImGui::Button("Apply optimize")) {
        AnimationPtr animation;
        if (item > 0)
        {
          SceneAsset sceneAsset;
          sceneAsset = load_scene(animationList[item].c_str(),
                                            SceneAsset::LoadScene::Skeleton | SceneAsset::LoadScene::Animation,
                                            animation_params.tolerance, animation_params.distance, &animation_info);
          if (!sceneAsset.animations.empty()) {
            animation = sceneAsset.animations[0];
          }
          animation_params.optimized = (animation_params.tolerance != 0.f || animation_params.distance != 0.f);
        }
        character.currentAnimation = animation;
        character.animTime = 0;
      }
      if (animation_params.optimized) {
        ImGui::Text("Optimized");
      } else {
        ImGui::Text("Non optimized");
      }
      ImGui::Text("Original: %llu", animation_info.original);
      ImGui::Text("Optimized: %llu", animation_info.optimized);
      ImGui::Text("Compressed: %llu", animation_info.compressed);
    }
    ImGui::End();

    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
    render_imguizmo(mCurrentGizmoOperation, mCurrentGizmoMode);

    const glm::mat4 &projection = scene->userCamera.projection;
    const glm::mat4 &transform = scene->userCamera.transform;
    mat4 cameraView = inverse(transform);
    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    
    glm::mat4 globNodeTm = character.transform;

    ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode,
                         glm::value_ptr(globNodeTm));

    character.transform = globNodeTm;
    break;
    }
}

void game_update()
{
  arcball_camera_update(
    scene->userCamera.arcballCamera,
    scene->userCamera.transform,
    get_delta_time());
  for (Character &character : scene->characters)
  {
    if (character.currentAnimation)
    {
      character.animTime += animation_params.pause ? 0 : get_delta_time() * animation_params.speed;
      if (character.animTime >= character.currentAnimation->duration()) {
        if (animation_params.loop) {
          character.animTime = 0;
        } else {
          character.animTime = character.currentAnimation->duration();
        }
      }

      // Samples optimized animation at t = animation_time_.
      ozz::animation::SamplingJob sampling_job;
      sampling_job.animation = character.currentAnimation.get();
      sampling_job.context = character.context_.get();
      sampling_job.ratio = character.animTime / character.currentAnimation->duration();
      sampling_job.output = ozz::make_span(character.locals_);
      if (!sampling_job.Run())
      {
        continue;
      }
    }
    else
    {
      auto restPose = character.skeleton_->joint_rest_poses();
      std::copy(restPose.begin(), restPose.end(), character.locals_.begin());
    }
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = character.skeleton_.get();
    ltm_job.input = ozz::make_span(character.locals_);
    ltm_job.output = ozz::make_span(character.models_);
    if (!ltm_job.Run())
    {
      continue;
    }
  }
}

static glm::mat4 to_glm(const ozz::math::Float4x4 &tm)
{
  glm::mat4 result;
  memcpy(glm::value_ptr(result), &tm.cols[0], sizeof(glm::mat4));
  return result;
}

void render_character(const Character &character, const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light)
{
  if (visualisation_params.show_mesh) {
    const Material &material = *character.material;
    const Shader &shader = material.get_shader();

    shader.use();
    material.bind_uniforms_to_shader();
    shader.set_mat4x4("Transform", character.transform);
    shader.set_mat4x4("ViewProjection", cameraProjView);
    shader.set_vec3("CameraPosition", cameraPosition);
    shader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
    shader.set_vec3("AmbientLight", light.ambient);
    shader.set_vec3("SunLight", light.lightColor);

    size_t boneNumber = character.mesh->bones.size();
    std::vector<mat4> bones(boneNumber);

    const auto &skeleton = *character.skeleton_;
    size_t nodeCount = skeleton.num_joints();
    for (size_t i = 0; i < nodeCount; i++)
    {
      auto it = character.mesh->bonesMap.find(skeleton.joint_names()[i]);
      if (it != character.mesh->bonesMap.end())
      {
        int boneIdx = it->second;
        bones[boneIdx] = to_glm(character.models_[i]) * character.mesh->bones[boneIdx].invBindPose;
      }
    }
    shader.set_mat4x4("Bones", bones);

    render(character.mesh);
  }
  
  const auto &skeleton = *character.skeleton_;
  size_t nodeCount = skeleton.num_joints();
  for (size_t i = 0; i < nodeCount; i++)
  { 
    float distanceToParent = 0.0f;

    int parentId = skeleton.joint_parents()[i];
    glm::mat4 boneTm   =  character.transform * to_glm(character.models_[i]);  
    if (parentId != -1 && parentId != 0 && parentId != 1) {
      glm::mat4 parentTm =  character.transform * to_glm(character.models_[parentId]);      
      vec3 parentPos = parentTm[3];
      vec3 bonePos = boneTm[3];
      distanceToParent = glm::length(bonePos - parentPos);
      if (visualisation_params.show_bones) {
        constexpr vec3 darkGreenColor = vec3(46.0 / 255, 142.0 / 255, 16.0 / 255);
        draw_bone(parentPos, bonePos, darkGreenColor, 0.1f);
      }
    }

    if (visualisation_params.show_arrows) {
      const float arrowLength = (distanceToParent + 0.1) / 5.0;
      constexpr float arrowSize = 0.004f;
      draw_arrow(boneTm, vec3(0), vec3(arrowLength, 0, 0), vec3(1, 0, 0), arrowSize);
      draw_arrow(boneTm, vec3(0), vec3(0, arrowLength, 0), vec3(0, 1, 0), arrowSize);
      draw_arrow(boneTm, vec3(0), vec3(0, 0, arrowLength), vec3(0, 0, 1), arrowSize);
    }
  }
}

void game_render()
{
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  const float grayColor = 0.3f;
  glClearColor(grayColor, grayColor, grayColor, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  const glm::mat4 &projection = scene->userCamera.projection;
  const glm::mat4 &transform = scene->userCamera.transform;
  glm::mat4 projView = projection * inverse(transform);

  for (const Character &character : scene->characters)
    render_character(character, projView, glm::vec3(transform[3]), scene->light);


  render_bones(projView, glm::vec3(transform[3]), scene->light);
  render_arrows(projView, glm::vec3(transform[3]), scene->light);
}

void close_game()
{
  scene.reset();
}

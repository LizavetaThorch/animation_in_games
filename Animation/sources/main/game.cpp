#include <render/direction_light.h>
#include <render/material.h>
#include <render/mesh.h>
#include <render/scene.h>
#include "camera.h"
#include <new_time.h>
#include <application.h>
#include <render/debug_arrow.h>
#include <render/debug_bones.h>
#include <imgui/imgui.h>
#include "ImGuizmo.h"

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
  RuntimeSkeleton skeleton;
};

struct Scene
{
  DirectionLight light;

  UserCamera userCamera;

  std::vector<Character> characters;

};

static std::unique_ptr<Scene> scene;

void game_init()
{
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
  scene->characters.emplace_back(Character{
    glm::identity<glm::mat4>(),
    sceneAsset.meshes[0],
    std::move(material),
    RuntimeSkeleton(sceneAsset.skeleton)});

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
    character.skeleton.updateLocalTransforms();
    const RuntimeSkeleton &skeleton = character.skeleton;
    size_t nodeCount = skeleton.ref->nodeCount;
    static size_t idx = 0;
    if (ImGui::Begin("Skeleton view"))
    {
      for (size_t i = 0; i < nodeCount; i++)
      {
        ImGui::Text("%d) %s", int(i), skeleton.ref->names[i].c_str());
        ImGui::SameLine();
        ImGui::PushID(i);
        if (ImGui::Button("edit"))
        {
          idx = i;
        }
        ImGui::PopID();
      }
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
    
    glm::mat4 globNodeTm = character.skeleton.globalTm[idx];

    ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode,
                         glm::value_ptr(globNodeTm));

    int parent = skeleton.ref->parent[idx];
    character.skeleton.localTm[idx] = glm::inverse(parent >= 0 ? character.skeleton.globalTm[parent] : glm::mat4(1.f)) * globNodeTm;

    break;
  }
}

void game_update()
{
  arcball_camera_update(
    scene->userCamera.arcballCamera,
    scene->userCamera.transform,
    get_delta_time());
}

void render_character(const Character &character, const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light)
{
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

  const RuntimeSkeleton &skeleton = character.skeleton;
  size_t nodeCount = skeleton.ref->nodeCount;
  for (size_t i = 0; i < nodeCount; i++)
  {
    auto it = character.mesh->bonesMap.find(skeleton.ref->names[i]);
    if (it != character.mesh->bonesMap.end())
    {
      int boneIdx = it->second;
      bones[boneIdx] = skeleton.globalTm[i] * character.mesh->bones[boneIdx].invBindPose;
    }
  }
  shader.set_mat4x4("Bones", bones);

  render(character.mesh);

  // for (const auto &bone : character.mesh->bones)
  for (size_t i = 0; i < nodeCount; i++)
  {
    float distanceToParent = 0.0f;

    int parentId = skeleton.ref->parent[i];
    if (parentId != -1 && parentId != 0 && parentId != 1) {
      vec3 parentPos = skeleton.globalTm[parentId][3];
      // vec3 parentPos = parentBone.bindPose[3];
      vec3 bonePos = skeleton.globalTm[i][3];
      distanceToParent = glm::length(bonePos - parentPos);
      constexpr vec3 darkGreenColor = vec3(46.0 / 255, 142.0 / 255, 16.0 / 255);
      draw_bone(parentPos, bonePos, darkGreenColor, 0.1f);
    }

    const float arrowLength = (distanceToParent + 0.1) / 5.0;
    constexpr float arrowSize = 0.004f;
    draw_arrow(skeleton.globalTm[i], vec3(0), vec3(arrowLength, 0, 0), vec3(1, 0, 0), arrowSize);
    draw_arrow(skeleton.globalTm[i], vec3(0), vec3(0, arrowLength, 0), vec3(0, 1, 0), arrowSize);
    draw_arrow(skeleton.globalTm[i], vec3(0), vec3(0, 0, arrowLength), vec3(0, 0, 1), arrowSize);
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

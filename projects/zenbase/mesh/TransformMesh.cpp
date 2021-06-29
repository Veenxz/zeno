#include <zeno/zen.h>
#include <zeno/MeshObject.h>
#include <zeno/NumericObject.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <cstring>

namespace zen {


static glm::vec3 mapplypos(glm::mat4 const &matrix, glm::vec3 const &vector) {
  auto vector4 = matrix * glm::vec4(vector, 1.0f);
  
  return glm::vec3(vector4) / vector4.w;
}


static glm::vec3 mapplydir(glm::mat4 const &matrix, glm::vec3 const &vector) {
  glm::mat3 normMatrix(matrix);
  normMatrix = glm::transpose(glm::inverse(normMatrix));
  auto vector3 = normMatrix * vector;
  return vector3;
}


struct TransformMesh : zen::INode {
  virtual void apply() override {
    auto mesh = get_input("mesh")->as<MeshObject>();
    auto outmesh = zen::IObject::make<MeshObject>();
    zen::vec3f translate = {0,0,0};
    zen::vec3f rotate = {0,0,0};
    zen::vec3f scaling = {1,1,1};
    if(has_input("translate"))
      translate = get_input("translate")->as<zen::NumericObject>()->get<zen::vec3f>();
    if(has_input("rotate"))
      rotate = get_input("rotate")->as<zen::NumericObject>()->get<zen::vec3f>();
    if(has_input("scaling"))
      scaling = get_input("scaling")->as<zen::NumericObject>()->get<zen::vec3f>();
    
    
    glm::mat4 matTrans = glm::translate(glm::vec3(translate[0], translate[1], translate[2]));
    glm::mat4 matRotx  = glm::rotate( rotate[0], glm::vec3(1,0,0) );
    glm::mat4 matRoty  = glm::rotate( rotate[1], glm::vec3(0,1,0) );
    glm::mat4 matRotz  = glm::rotate( rotate[2], glm::vec3(0,0,1) );
    glm::mat4 matScal  = glm::scale( glm::vec3(scaling[0], scaling[1], scaling[2] ));
    auto matrix = matRotz*matRoty*matRotx*matScal*matTrans;
    for (auto const &x: mesh->vertices) {
      
      outmesh->vertices.push_back(mapplypos(matrix, x));
    }
    for (auto const &x: mesh->uvs) {
      outmesh->uvs.push_back(x);
    }
    for (auto const &x: mesh->normals) {
      outmesh->normals.push_back(glm::normalize(mapplydir(matrix, x)));
    }
    set_output("mesh", outmesh);
  }
};


static int defTransformMesh = zen::defNodeClass<TransformMesh>("TransformMesh",
    { /* inputs: */ {
    "mesh",
    "translate",
    "rotate",
    "scaling",
    }, /* outputs: */ {
    "mesh",
    }, /* params: */ {
    }, /* category: */ {
    "trimesh",
    }});

}


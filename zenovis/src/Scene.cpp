#include <zenovis/Scene.h>
#include <zenovis/Camera.h>
#include <zenovis/IGraphic.h>
#include <zenovis/DrawOptions.h>
#include <zenovis/RenderEngine.h>
#include <zenovis/GraphicsManager.h>
#include <zenovis/ShaderManager.h>
#include <zenovis/opengl/buffer.h>
#include <zenovis/opengl/common.h>
#include <zenovis/opengl/vao.h>
#include <zeno/utils/scope_exit.h>
#include <map>

namespace zenovis {

void Scene::loadGLAPI(void *procaddr) {
    int res = gladLoadGLLoader((GLADloadproc)procaddr);
    if (res < 0)
        zeno::log_error("failed to load OpenGL via GLAD: {}", res);
}

Scene::~Scene() = default;

Scene::Scene()
    : camera(std::make_unique<Camera>()),
      drawOptions(std::make_unique<DrawOptions>()),
      shaderMan(std::make_unique<ShaderManager>()) {

    auto version = (const char *)glGetString(GL_VERSION);
    zeno::log_info("OpenGL version: {}", version ? version : "(null)");

    hudGraphics.push_back(makeGraphicGrid(this));
    hudGraphics.push_back(makeGraphicAxis(this));

    renderEngine = makeRenderEngineZhxx(this);
}

void Scene::setObjects(std::vector<std::shared_ptr<zeno::IObject>> const &objs) {
    this->objects = objs;
    if (renderEngine)
        renderEngine->update();
}

void Scene::draw() {
    CHECK_GL(glViewport(0, 0, camera->m_nx, camera->m_ny));
    CHECK_GL(glClearColor(drawOptions->bgcolor.r, drawOptions->bgcolor.g, drawOptions->bgcolor.b, 0.0f));

    if (renderEngine)
        renderEngine->draw();
}

std::vector<char> Scene::record_frame_offline(int hdrSize, int rgbComps) {
    auto hdrType = std::map<int, int>{
        {1, GL_UNSIGNED_BYTE},
        {2, GL_HALF_FLOAT},
        {4, GL_FLOAT},
    }.at(hdrSize);
    auto rgbType = std::map<int, int>{
        {1, GL_RED},
        {2, GL_RG},
        {3, GL_RGB},
        {4, GL_RGBA},
    }.at(rgbComps);

    std::vector<char> pixels(camera->m_nx * camera->m_ny * rgbComps * hdrSize);

    GLuint fbo, rbo1, rbo2;
    CHECK_GL(glGenRenderbuffers(1, &rbo1));
    CHECK_GL(glGenRenderbuffers(1, &rbo2));
    CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, rbo1));
    CHECK_GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, camera->m_nx,
                                   camera->m_ny));
    CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, rbo2));
    CHECK_GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F,
                                   camera->m_nx, camera->m_ny));

    CHECK_GL(glGenFramebuffers(1, &fbo));
    CHECK_GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo));
    CHECK_GL(glFramebufferRenderbuffer(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo1));
    CHECK_GL(glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                       GL_RENDERBUFFER, rbo2));
    CHECK_GL(glDrawBuffer(GL_COLOR_ATTACHMENT0));
    CHECK_GL(glClearColor(drawOptions->bgcolor.r, drawOptions->bgcolor.g,
                          drawOptions->bgcolor.b, 0.0f));
    CHECK_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    CHECK_GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo));
    draw();

    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        CHECK_GL(glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo));
        CHECK_GL(glBlitFramebuffer(0, 0, camera->m_nx, camera->m_ny, 0, 0,
                                   camera->m_nx, camera->m_ny, GL_COLOR_BUFFER_BIT,
                                   GL_NEAREST));
        CHECK_GL(glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo));
        CHECK_GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        CHECK_GL(glReadBuffer(GL_COLOR_ATTACHMENT0));

        CHECK_GL(glReadPixels(0, 0, camera->m_nx, camera->m_ny, rgbType,
                              hdrType, pixels.data()));
    }

    CHECK_GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
    CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    CHECK_GL(glDeleteRenderbuffers(1, &rbo1));
    CHECK_GL(glDeleteRenderbuffers(1, &rbo2));
    CHECK_GL(glDeleteFramebuffers(1, &fbo));

    return pixels;
}

} // namespace zenovis

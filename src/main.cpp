#include "sokol/sokol_app.h"
#include "sokol/sokol_audio.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"

#include "chip8.hpp"

static Chip8 emu;

static sg_pass_action pass_action;
static sg_image screen_tex;
static sg_sampler screen_smp;
static sg_pipeline pip;
static sg_bindings bind;

static uint32_t smp_pos = 0;

void audio_cb(float *buf, int num_fr, int num_ch) {
  const int freq = 440;
  const int smp_rate = saudio_sample_rate();
  const int per = smp_rate / freq;

  for (int i = 0; i < num_fr; ++i) {
    float smp = 0.0f;

    if (emu.getSoundTimer() > 0)
      smp = (smp_pos % per) < (per / 2) ? 0.05f : -0.05f;

    for (int ch = 0; ch < num_ch; ++ch)
      buf[i * num_ch + ch] = smp;

    smp_pos++;
  }
}

void init(void) {
  saudio_desc audio_desc = {};
  audio_desc.stream_cb = audio_cb;
  saudio_setup(&audio_desc);

  sg_desc desc = {};
  desc.environment = sglue_environment();
  sg_setup(&desc);

  pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
  pass_action.colors[0].clear_value = {0.1f, 0.1, 0.2f, 1.0f};

  sg_image_desc img_desc = {};
  img_desc.width = 64;
  img_desc.height = 32;
  img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
  img_desc.usage.immutable = false;
  img_desc.usage.stream_update = true;
  screen_tex = sg_make_image(&img_desc);

  sg_sampler_desc smp_desc = {};
  smp_desc.min_filter = SG_FILTER_NEAREST;
  smp_desc.mag_filter = SG_FILTER_NEAREST;
  smp_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  smp_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  screen_smp = sg_make_sampler(&smp_desc);

  float vertices[] = {
      -1.0f, 1.0f,  0.0f, 0.0f, // LT
      1.0f,  1.0f,  1.0f, 0.0f, // RT
      1.0f,  -1.0f, 1.0f, 1.0f, // RB
      -1.0f, -1.0f, 0.0f, 1.0f, // LB
  };
  uint16_t indices[] = {0, 1, 2, 0, 2, 3};

  sg_buffer_desc vbuf_desc = {};
  vbuf_desc.data = sg_range(vertices, sizeof(vertices));
  bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

  sg_buffer_desc ibuf_desc = {};
  ibuf_desc.usage.index_buffer = true;
  ibuf_desc.data = sg_range(indices, sizeof(indices));
  bind.index_buffer = sg_make_buffer(&ibuf_desc);

  sg_shader_desc shd_desc = {};
  shd_desc.vertex_func.source = "#version 330\n"
                                "layout(location=0) in vec2 position;\n"
                                "layout(location=1) in vec2 texcoord0;\n"
                                "out vec2 uv;\n"
                                "void main() {\n"
                                "  gl_Position = vec4(position, 0.0, 1.0);\n"
                                "  uv = texcoord0;\n"
                                "}\n";
  shd_desc.fragment_func.source = "#version 330\n"
                                  "uniform sampler2D tex;\n"
                                  "in vec2 uv;\n"
                                  "out vec4 frag_color;\n"
                                  "void main() {\n"
                                  "  frag_color = texture(tex, uv);\n"
                                  "}\n";
  shd_desc.views[0].texture.stage = SG_SHADERSTAGE_FRAGMENT;
  shd_desc.samplers[0].stage = SG_SHADERSTAGE_FRAGMENT;
  shd_desc.texture_sampler_pairs[0].stage = SG_SHADERSTAGE_FRAGMENT;
  shd_desc.texture_sampler_pairs[0].glsl_name = "tex";
  shd_desc.texture_sampler_pairs[0].view_slot = 0;
  shd_desc.texture_sampler_pairs[0].sampler_slot = 0;

  sg_pipeline_desc pip_desc = {};
  pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2; // xy
  pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2; // uv
  pip_desc.shader = sg_make_shader(&shd_desc);
  pip_desc.index_type = SG_INDEXTYPE_UINT16;
  pip = sg_make_pipeline(&pip_desc);

  sg_view_desc view_desc = {};
  view_desc.texture.image = screen_tex;
  bind.views[0] = sg_make_view(&view_desc);
  bind.samplers[0] = screen_smp;
}

void frame(void) {
  // ~700ops/sec
  for (int i = 0; i < emu.getCyclesPerFrame(); ++i)
    emu.cycle();

  // 60hz timers
  emu.updateTimers();

  sg_image_data data = {};
  data.mip_levels[0].ptr = emu.getDisplayData();
  data.mip_levels[0].size = 64 * 32 * sizeof(uint32_t);
  sg_update_image(screen_tex, &data);

  sg_pass pass = {};
  pass.action = pass_action;
  pass.swapchain = sglue_swapchain();

  sg_begin_pass(&pass);
  sg_apply_pipeline(pip);
  sg_apply_bindings(&bind);
  sg_draw(0, 6, 1);
  sg_end_pass();
  sg_commit();
}

void cleanup(void) {
  saudio_shutdown();
  sg_shutdown();
}

int map(sapp_keycode k) {
  switch (k) {
  case SAPP_KEYCODE_1:
    return 0x1;
  case SAPP_KEYCODE_2:
    return 0x2;
  case SAPP_KEYCODE_3:
    return 0x3;
  case SAPP_KEYCODE_4:
    return 0xC;
  case SAPP_KEYCODE_Q:
    return 0x4;
  case SAPP_KEYCODE_W:
    return 0x5;
  case SAPP_KEYCODE_E:
    return 0x6;
  case SAPP_KEYCODE_R:
    return 0xD;
  case SAPP_KEYCODE_A:
    return 0x7;
  case SAPP_KEYCODE_S:
    return 0x8;
  case SAPP_KEYCODE_D:
    return 0x9;
  case SAPP_KEYCODE_F:
    return 0xE;
  case SAPP_KEYCODE_Z:
    return 0xA;
  case SAPP_KEYCODE_X:
    return 0x0;
  case SAPP_KEYCODE_C:
    return 0xB;
  case SAPP_KEYCODE_V:
    return 0xF;
  default:
    return -1;
  }
}

void event(const sapp_event *ev) {
  if (ev->type == SAPP_EVENTTYPE_KEY_DOWN ||
      ev->type == SAPP_EVENTTYPE_KEY_UP) {
    int chip8_k = map(ev->key_code);
    if (chip8_k != -1)
      emu.setKey(chip8_k, ev->type == SAPP_EVENTTYPE_KEY_DOWN);
  }
}

sapp_desc sokol_main(int argc, char *argv[]) {
  // A way to silence "unused variable" warnings
  (void)argc;
  (void)argv;

  emu.loadROM(argv[1]);

  sapp_desc app = {};
  app.init_cb = init;
  app.frame_cb = frame;
  app.cleanup_cb = cleanup;
  app.event_cb = event;

  app.width = 640;
  app.height = 320;
  app.sample_count = 1;
  app.high_dpi = true;
  app.window_title = "CHIP-8 Sokol";

  return app;
}

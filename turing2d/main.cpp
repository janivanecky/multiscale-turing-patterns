#include "platform.h"
#include "graphics.h"
#include "file_system.h"
#include "maths.h"
#include "memory.h"
#include "ui.h"
#include "font.h"
#include "input.h"
#include "random.h"
#include <cassert>
#include <mmsystem.h>
#include "logging.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

float quad_vertices[] = {
    -1.0f, -1.0f, 0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f,

    -1.0f, -1.0f, 0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f,
};

uint32_t quad_vertices_stride = sizeof(float) * 6;
uint32_t quad_vertices_count = 6;

ComputeShader get_compute_shader(char *file_path) {
    File shader_file = file_system::read_file(file_path);
    ComputeShader shader = graphics::get_compute_shader_from_code((char *)shader_file.data, shader_file.size);
    file_system::release_file(shader_file);
    assert(graphics::is_ready(&shader));
    return shader;
}

Vector3 hsv_to_rgb(float h, float s, float v) {
    Vector3 out;
    if(s <= 0.0) {
        out.x = v;
        out.y = v;
        out.z = v;
        return out;
    }
    float hh = h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    int i = (int)hh;
    float ff = hh - i;
    float p = v * (1.0 - s);
    float q = v * (1.0 - (s * ff));
    float t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.x = v;
        out.y = t;
        out.z = p;
        break;
    case 1:
        out.x = q;
        out.y = v;
        out.z = p;
        break;
    case 2:
        out.x = p;
        out.y = v;
        out.z = t;
        break;

    case 3:
        out.x = p;
        out.y = q;
        out.z = v;
        break;
    case 4:
        out.x = t;
        out.y = p;
        out.z = v;
        break;
    case 5:
    default:
        out.x = v;
        out.y = p;
        out.z = q;
        break;
    }
    return out;     
}

#define FAST_DIFFUSE
int main(int argc, char **argv) {
    // Set up window
    const int NO_TEXTURES = 2;
    const int NO_SCALES = 8;
    uint32_t window_width = 1024, window_height = 1024;
    //uint32_t window_width = 2048, window_height = 2048;
 	Window window = platform::get_window("Turing Patterns", window_width, window_height);
    uint32_t world_width = window_width * 2, world_height = window_height * 2;
    assert(platform::is_window_valid(&window));

    // Init graphics
    graphics::init();
    graphics::init_swap_chain(&window);

    font::init();
    ui::init((float)window_width, (float)window_height);
    ui::set_input_responsive(true);

    // Create window render target
	RenderTarget render_target_window = graphics::get_render_target_window(false);
    assert(graphics::is_ready(&render_target_window));
    DepthBuffer depth_buffer = graphics::get_depth_buffer(window_width, window_height);
    assert(graphics::is_ready(&depth_buffer));
    graphics::set_render_targets_viewport(&render_target_window, &depth_buffer);

    // Compute shaders init.
    #ifdef FAST_DIFFUSE
    ComputeShader pyramid_shader = get_compute_shader("pyramid_shader.hlsl");
    ComputeShader diffuse_pyramid_shader = get_compute_shader("diffuse_shader_pyramid.hlsl");
    #else
    ComputeShader diffuse_shader_x_init = get_compute_shader("diffuse_shader_x_init.hlsl");
    ComputeShader diffuse_shader_x = get_compute_shader("diffuse_shader_x.hlsl");
    ComputeShader diffuse_shader_y = get_compute_shader("diffuse_shader_y.hlsl");
    #endif
    ComputeShader symmetry_shader = get_compute_shader("symmetry_shader.hlsl");
    ComputeShader normalize_shader = get_compute_shader("normalize_shader.hlsl");
    ComputeShader update_shader = get_compute_shader("update_shader.hlsl");
    ComputeShader zoom_shader = get_compute_shader("zoom_shader.hlsl");

    // Vertex shader for displaying textures.
    File vertex_shader_file = file_system::read_file("vertex_shader.hlsl"); 
    VertexShader vertex_shader_2d = graphics::get_vertex_shader_from_code((char *)vertex_shader_file.data, vertex_shader_file.size);
    file_system::release_file(vertex_shader_file);
    assert(graphics::is_ready(&vertex_shader_2d));

    // Pixel shader for displaying textures.
    File pixel_shader_file = file_system::read_file("pixel_shader.hlsl"); 
    PixelShader pixel_shader_2d = graphics::get_pixel_shader_from_code((char *)pixel_shader_file.data, pixel_shader_file.size);
    file_system::release_file(pixel_shader_file);
    assert(graphics::is_ready(&pixel_shader_2d));
    
    // Textures
    float *grid_values = memory::alloc_heap<float>(world_width * world_height);
    for (int i = 0; i < world_width * world_height; ++i) {
        grid_values[i] = random::uniform();
    }
    float *color_values = memory::alloc_heap<float>(world_width * world_height * 4);
    for (int i = 0; i < world_width * world_height * 4; ++i) {
        color_values[i] = 0.0f;
    }
    Texture2D screenshot_texture = graphics::get_texture2D(NULL, window_width, window_height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 4, true);

    Texture2D grid = graphics::get_texture2D(grid_values, world_width, world_height, DXGI_FORMAT_R32_FLOAT, 4);
    Texture2D grid_colors = graphics::get_texture2D(color_values, world_width, world_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);

    #ifdef FAST_DIFFUSE
    Texture2D scale_texture = graphics::get_texture2D(NULL, world_width * 2, world_height, DXGI_FORMAT_R32_FLOAT, 4);
    #endif
    Texture2D temp_texture_color = graphics::get_texture2D(NULL, world_width, world_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    Texture2D temp_texture_grid = graphics::get_texture2D(NULL, world_width, world_height, DXGI_FORMAT_R32_FLOAT, 4);

    Texture2D activators[NO_TEXTURES];
    for (int t = 0; t < NO_TEXTURES; t++) {
        activators[t] = graphics::get_texture2D(NULL, world_width, world_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    }
    Texture2D activators_symmetry[NO_TEXTURES];
    for (int t = 0; t < NO_TEXTURES; t++) {
        activators_symmetry[t] = graphics::get_texture2D(NULL, world_width, world_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    }
    Texture2D inhibitors[NO_TEXTURES];
    for (int t = 0; t < NO_TEXTURES; t++) {
        inhibitors[t] = graphics::get_texture2D(NULL, world_width, world_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    };
    Texture2D inhibitors_symmetry[NO_TEXTURES];
    for (int t = 0; t < NO_TEXTURES; t++) {
        inhibitors_symmetry[t] = graphics::get_texture2D(NULL, world_width, world_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    };
	graphics::set_blend_state(BlendType::ALPHA);
    TextureSampler tex_sampler = graphics::get_texture_sampler(SampleMode::BORDER);

    // Quad mesh for rendering the resulting texture.
    Mesh quad_mesh = graphics::get_mesh(quad_vertices, quad_vertices_count, quad_vertices_stride, NULL, 0, 0);

    // Constant buffers.
    struct Config {
        int world_width;
        int world_height;
        float update_speed;
        float color_update_speed;
        float zoom_ratio;
        int iterations_per_zoom;
        int show_color;
        int wrap_bounds;
    };
    Config config = {
        world_width,
        world_height,
        1.0f,
        1.0f,
        0.98f,
        1,
        true,
        true,
    };
    ConstantBuffer config_buffer = graphics::get_constant_buffer(sizeof(Config));

    float update_vals[NO_SCALES] = {
        0.1f, 0.08f, 0.06f, 0.05f,
        0.04f, 0.03f, 0.02f, 0.01f,
    };
    ConstantBuffer update_vals_buffer = graphics::get_constant_buffer(sizeof(float) * NO_SCALES);

    Vector4 colors[8] = {
        Vector4(159,111,99,1) / 255.0f,
        Vector4(45,69,88,1) / 255.0f,
        Vector4(152,207,184,1) / 255.0f,
        Vector4(220,231,180,1) / 255.0f,
        Vector4(1,1,0,1),
        Vector4(1,0,1,1),
        Vector4(0.5,1,0.2,1),
        Vector4(0,1,0.5,1),
    };
    ConstantBuffer color_buffer = graphics::get_constant_buffer(sizeof(Vector4) * 8);

    int symmetries[8] = {
        3, 3, 3, 3, 3, 3, 3, 3,
    };
    ConstantBuffer symmetry_buffer = graphics::get_constant_buffer(sizeof(int) * 4);

    ConstantBuffer pyramid_buffer = graphics::get_constant_buffer(sizeof(int) * 4);

    Timer timer = timer::get();
    timer::start(&timer);

    int activator_diffuse_sizes[NO_SCALES] = {
        250, 200, 100, 50, 25, 10, 5, 1,
    };
    int inhibitor_diffuse_sizes[NO_SCALES] = {
        500, 400, 200, 100, 50, 20, 10, 2,
    };

    ConstantBuffer kernel_size_buffer = graphics::get_constant_buffer(sizeof(int) * 4);

    // Render loop
    bool is_running = true;
    bool record = false;
    bool screenshot_folder_created = false;
    char screenshot_folder_name[100];
    bool pause = false;
    bool zoom = false;
    bool show_ui = false;
    int screenshot_counter = 0;
    
    while(is_running)
    {
        input::reset();
    
        // Event loop
        Event event;
        while(platform::get_event(&event))
        {
            input::register_event(&event);

            // Check if close button pressed
            switch(event.type)
            {
                case EventType::EXIT:
                {
                    is_running = false;
                }
                break;
            }
        }
        // React to inputs
        {
            if (input::key_pressed(KeyCode::ESC)) is_running = false; 
            if (input::key_pressed(KeyCode::F5)) record = !record; 
            if (input::key_pressed(KeyCode::F4)) pause = !pause;
            if (input::key_pressed(KeyCode::F1)) show_ui = !show_ui;
            if (input::key_pressed(KeyCode::F8) || record) {
                if (!screenshot_folder_created) {
                    SYSTEMTIME time_string = platform::get_datetime();
                    sprintf(screenshot_folder_name, "%d_%d_%d_%d_%d_%d", time_string.wYear, time_string.wMonth,
                            time_string.wDay, time_string.wHour, time_string.wMinute, time_string.wSecond);
                    file_system::mkdir(screenshot_folder_name);
                    screenshot_folder_created = true;
                }

                char file_path[100];
                sprintf(file_path, "%s/%d.png", screenshot_folder_name, screenshot_counter);
                
                D3D11_MAP map_type = D3D11_MAP_READ;
                D3D11_MAPPED_SUBRESOURCE mappedResource;

                graphics_context->context->CopyResource(screenshot_texture.texture, render_target_window.texture);
                graphics_context->context->Map(screenshot_texture.texture, 0, map_type, NULL, &mappedResource);
                BYTE* bytes = (BYTE*)mappedResource.pData;
                unsigned int pitch = mappedResource.RowPitch;
                stbi_write_png(file_path, window_width, window_height, 4, bytes, pitch);
                graphics_context->context->Unmap(screenshot_texture.texture, 0);
                screenshot_counter++;
                if(screenshot_counter == 1800) {
                    is_running = false;
                }
            }
            if (input::key_pressed(KeyCode::F2)) {
                int max_diffuse_values[8] = {
                    150, 125, 100, 100, 50, 25, 10, 5
                };
                float max_inhibitor_to_activator_ratios[8] = {
                    3.0f, 3.0f, 3.0f, 4.0f, 4.0f, 4.0f, 4.0f
                };
                int random_symmetry = (int)random::uniform(1, 9);
                for(int i = 0; i < NO_SCALES; ++i) {
                    activator_diffuse_sizes[i] = random::uniform(1, max_diffuse_values[i]);
                    inhibitor_diffuse_sizes[i] = activator_diffuse_sizes[i] * random::uniform(1.5f, max_inhibitor_to_activator_ratios[i]);

                    symmetries[i] = random_symmetry;
                    update_vals[i] = i < 4 ? random::uniform(-0.03f, 0.1f) : random::uniform(-0.01f, 0.05);
                }

                graphics::release(&grid);
                grid = graphics::get_texture2D(grid_values, world_width, world_height, DXGI_FORMAT_R32_FLOAT, 4);
                graphics::release(&grid_colors);
                grid_colors = graphics::get_texture2D(color_values, world_width, world_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
            }
            if (input::key_pressed(KeyCode::F3)) {
                for(int i = 0; i < NO_SCALES; ++i) {
                    Vector3 color = hsv_to_rgb(random::uniform(0, 360), 1.0f, 1.0f);
                    if (i >= 4) {
                        color = color * 0.5f;
                    }
                    colors[NO_SCALES - i - 1].x = color.x;
                    colors[NO_SCALES - i - 1].y = color.y;
                    colors[NO_SCALES - i - 1].z = color.z;

                }

                graphics::release(&grid_colors);
                grid_colors = graphics::get_texture2D(color_values, world_width, world_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
            }
        }

        // Set constant buffers.
        graphics::set_constant_buffer(&config_buffer, 0);
        graphics::update_constant_buffer(&config_buffer, &config);
        graphics::set_constant_buffer(&color_buffer, 1);
        graphics::update_constant_buffer(&color_buffer, &colors);
        graphics::set_constant_buffer(&symmetry_buffer, 2);
        graphics::set_constant_buffer(&update_vals_buffer, 4);
        graphics::update_constant_buffer(&update_vals_buffer, &update_vals);

        if (!pause) {
        for(int x = 0; x < config.iterations_per_zoom; ++x) {
#ifdef FAST_DIFFUSE
            // Create mipmaps
            graphics::set_constant_buffer(&pyramid_buffer, 3);
            graphics::set_compute_shader(&pyramid_shader);
            graphics::set_texture_compute(&grid, 0);
            graphics::set_texture_compute(&scale_texture, 1);
            for (int i = world_width, j = 0; i > 0; i /= 2, j++) {
                int pyramid_level = j;
                graphics::update_constant_buffer(&pyramid_buffer, &pyramid_level);
                uint32_t group_size = i >= 32 ? i / 32 : 1;
                graphics::run_compute(group_size, group_size, 1);
            }
            graphics::unset_texture_compute(1);

            graphics::set_compute_shader(&diffuse_pyramid_shader);
            graphics::set_constant_buffer(&kernel_size_buffer, 3);
            graphics::set_texture_compute(&scale_texture, 0);

            // Difuse activators
            for (int t = 0; t < NO_TEXTURES; ++t) {
                int kernel_sizes[4] = {
                    activator_diffuse_sizes[t * 4 + 0],
                    activator_diffuse_sizes[t * 4 + 1],
                    activator_diffuse_sizes[t * 4 + 2],
                    activator_diffuse_sizes[t * 4 + 3],
                };
                graphics::update_constant_buffer(&kernel_size_buffer, &kernel_sizes);
                graphics::set_texture_compute(&activators[t], 1);
                graphics::run_compute(world_width / 32, world_height / 32, 1);
                graphics::unset_texture_compute(1);
            }

            // Difuse inhibitors
            for (int t = 0; t < NO_TEXTURES; ++t) {
                int kernel_sizes[4] = {
                    inhibitor_diffuse_sizes[t * 4 + 0],
                    inhibitor_diffuse_sizes[t * 4 + 1],
                    inhibitor_diffuse_sizes[t * 4 + 2],
                    inhibitor_diffuse_sizes[t * 4 + 3],
                };
                graphics::update_constant_buffer(&kernel_size_buffer, &kernel_sizes);
                graphics::set_texture_compute(&inhibitors[t], 1);
                graphics::run_compute(world_width / 32, world_height / 32, 1);
                graphics::unset_texture_compute(1);
            }
#else
            // Diffuse activators
            #define BLUR_COUNT 3
            auto get_size = [](int s, int i) {
                int d = s / BLUR_COUNT;
                int r = s % BLUR_COUNT;
                return d + (r + i) / BLUR_COUNT;
            };
            graphics::set_constant_buffer(&kernel_size_buffer, 3);
            for (int t = 0; t < NO_TEXTURES; ++t) {
                for (int i = BLUR_COUNT - 1; i >= 0; --i) {
                    int kernel_sizes[4] = {
                        (t * 4 + 0) < NO_SCALES ? get_size(activator_diffuse_sizes[t * 4 + 0], i) : 0,
                        (t * 4 + 1) < NO_SCALES ? get_size(activator_diffuse_sizes[t * 4 + 1], i) : 0,
                        (t * 4 + 1) < NO_SCALES ? get_size(activator_diffuse_sizes[t * 4 + 2], i) : 0,
                        (t * 4 + 2) < NO_SCALES ? get_size(activator_diffuse_sizes[t * 4 + 3], i) : 0,
                    };
                    graphics::update_constant_buffer(&kernel_size_buffer, &kernel_sizes);
                    if(i == BLUR_COUNT - 1) {
                        graphics::set_compute_shader(&diffuse_shader_x_init);
                        graphics::set_texture_compute(&grid, 0);
                    } else {
                        graphics::set_compute_shader(&diffuse_shader_x);
                        graphics::set_texture_compute(&activators[t], 0);
                    } 
                    graphics::set_texture_compute(&temp_texture_color, 1);
                    graphics::run_compute(world_width / 32, world_height / 32, 1);
                    graphics::unset_texture_compute(1);

                    graphics::set_compute_shader(&diffuse_shader_y);
                    graphics::set_texture_compute(&temp_texture_color, 0);
                    graphics::set_texture_compute(&activators[t], 1);
                    graphics::run_compute(world_width / 32, world_height / 32, 1);
                    graphics::unset_texture_compute(1);
                }
            }

            // Difuse inhibitors
            for (int t = 0; t < NO_TEXTURES; ++t) {
                for (int i = BLUR_COUNT - 1; i >= 0; --i) {
                    int kernel_sizes[4] = {
                        (t * 4 + 0) < NO_SCALES ? get_size(inhibitor_diffuse_sizes[t * 4 + 0], i) : 0,
                        (t * 4 + 1) < NO_SCALES ? get_size(inhibitor_diffuse_sizes[t * 4 + 1], i) : 0,
                        (t * 4 + 1) < NO_SCALES ? get_size(inhibitor_diffuse_sizes[t * 4 + 2], i) : 0,
                        (t * 4 + 2) < NO_SCALES ? get_size(inhibitor_diffuse_sizes[t * 4 + 3], i) : 0,
                    };
                    graphics::update_constant_buffer(&kernel_size_buffer, &kernel_sizes);
                    if(i == BLUR_COUNT - 1) {
                        graphics::set_compute_shader(&diffuse_shader_x_init);
                        graphics::set_texture_compute(&grid, 0);
                    } else {
                        graphics::set_compute_shader(&diffuse_shader_x);
                        graphics::set_texture_compute(&inhibitors[t], 0);
                    } 
                    graphics::set_texture_compute(&temp_texture_color, 1);
                    graphics::run_compute(world_width / 32, world_height / 32, 1);
                    graphics::unset_texture_compute(1);
                    
                    graphics::set_compute_shader(&diffuse_shader_y);
                    graphics::set_texture_compute(&temp_texture_color, 0);
                    graphics::set_texture_compute(&inhibitors[t], 1);
                    graphics::run_compute(world_width / 32, world_height / 32, 1);
                    graphics::unset_texture_compute(1);
                }
            }
        
    #endif
            // Make activator and inhibitor fields symmetric.
            graphics::set_compute_shader(&symmetry_shader);
            for(int i = 0; i < NO_TEXTURES; ++i) {
                graphics::update_constant_buffer(&symmetry_buffer, symmetries + i * 4);
                graphics::set_texture_compute(&activators[i], 0);
                graphics::set_texture_compute(&activators_symmetry[i], 1);
                graphics::run_compute(world_width / 32, world_height / 32, 1);
                
                graphics::set_texture_compute(&inhibitors[i], 0);
                graphics::set_texture_compute(&inhibitors_symmetry[i], 1);
                graphics::run_compute(world_width / 32, world_height / 32, 1);
            }

            // Update grid cells.
            graphics::set_compute_shader(&update_shader);
            graphics::set_texture_compute(&activators_symmetry[0], 0);
            graphics::set_texture_compute(&activators_symmetry[1], 1);
            graphics::set_texture_compute(&inhibitors_symmetry[0], 2);
            graphics::set_texture_compute(&inhibitors_symmetry[1], 3);
            graphics::set_texture_compute(&grid, 4);
            graphics::set_texture_compute(&grid_colors, 5);
            
            graphics::run_compute(world_width / 32, world_height / 32, 1);

            graphics::unset_texture_compute(0);
            graphics::unset_texture_compute(1);
            graphics::unset_texture_compute(2);
            graphics::unset_texture_compute(3);
            graphics::unset_texture_compute(4);
            graphics::unset_texture_compute(5);

            // Normalize grid values to 0-1 range.
            graphics::set_compute_shader(&normalize_shader);
            graphics::set_texture_compute(&grid, 0);
            graphics::run_compute(1, 1, 1);
        }

        if(zoom) {
            graphics_context->context->CopyResource(temp_texture_grid.texture, grid.texture);
            graphics::set_compute_shader(&zoom_shader);
            graphics::set_texture_compute(&temp_texture_grid, 0);
            graphics::set_texture_compute(&grid, 1);
            graphics::run_compute(world_width / 32, world_height / 32, 1);

            graphics_context->context->CopyResource(temp_texture_color.texture, grid_colors.texture);
            graphics::set_compute_shader(&zoom_shader);
            graphics::set_texture_compute(&temp_texture_color, 0);
            graphics::set_texture_compute(&grid_colors, 1);
            graphics::run_compute(world_width / 32, world_height / 32, 1);
        }

        graphics::unset_texture_compute(0);
        graphics::unset_texture_compute(1);

        // Clear screen.
        graphics::set_render_targets_viewport(&render_target_window);
        graphics::clear_render_target(&render_target_window, 0.0f, 0.0f, 0.0f, 1);
            
        // Draw.
        graphics::set_pixel_shader(&pixel_shader_2d);
        graphics::set_vertex_shader(&vertex_shader_2d);
        graphics::set_texture_sampler(&tex_sampler, 0);
        graphics::set_texture(&grid_colors, 0);
        graphics::set_texture(&grid, 1);

        graphics::draw_mesh(&quad_mesh);

        graphics::unset_texture(0);
        graphics::unset_texture(1);

        if(show_ui) {
            Panel panel = ui::start_panel("", Vector2(10, 10.0f), 420.0f);
            ui::add_slider(&panel, "UPDATE SPEED", &config.update_speed, 0.0, 1.0f);
            ui::add_slider(&panel, "COL UPDATE SPEED", &config.color_update_speed, 0.0, 1.0f);
            ui::add_slider(&panel, "ZOOM RATIO", &config.zoom_ratio, 0.5f, 1.5f);
            ui::add_toggle(&panel, "ZOOM", &zoom);
            ui::add_toggle(&panel, "WRAP", &config.wrap_bounds);
            ui::add_toggle(&panel, "SHOW COLORS", &config.show_color);
            float iterations_per_zoom = config.iterations_per_zoom;
            ui::add_slider(&panel, "ITERATIONS PER ZOOM", &iterations_per_zoom, 1, 50);
            config.iterations_per_zoom = iterations_per_zoom;
            ui::end_panel(&panel);
            ui::end();
        }

        graphics::swap_frames();
        }

    }

    ui::release();

    //graphics::show_live_objects();

    graphics::release();

    return 0;
}
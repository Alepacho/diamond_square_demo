#include <iostream>
#include <vector>
#include <functional>
#include <random>
#include <ctime>

#include "imgui.h"
#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_sdlrenderer.h"

#include <SDL2/SDL.h>

std::string random_string(std::string::size_type length)
{
    static auto& chrs = "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;
    s.reserve(length);
    while(length--) s += chrs[pick(rg)];

    return s;
}

bool running;
void log(std::string str) {
    std::cout << str << std::endl;
}

template<typename T>
void log(std::string str1, T var) {
    std::cout << str1 << var << std::endl;
}

void error(std::string str) {
    std::cout << "[ERROR] " << str << std::endl;
    running = false;
}

struct Window {
    SDL_Window* data;
    int width, height;
    Window() : width(1024), height(768) {};
} window;

struct Render {
    SDL_Renderer* data;

    void clear() { clear((SDL_Color){ 0, 0, 0 }); }
    void clear(SDL_Color c) { SDL_SetRenderDrawColor(data, c.r, c.g, c.b, 255); SDL_RenderClear(data); }

    void pixel(int x, int y) { SDL_RenderDrawPoint(data, x, y); }
    void pixel(int x, int y, SDL_Color c) { SDL_SetRenderDrawColor(data, c.r, c.g, c.b, 255); SDL_RenderDrawPoint(data, x, y); }
} render;

class Terrain {
    private:
        struct Texture {
            int width, height;
            SDL_Texture* data;
        } texture;
        std::vector<std::vector<float> > data;
        std::hash<std::string> hasher;
        std::mt19937 gen;
        float random() {
            std::uniform_int_distribution<float> pick(-options.height, options.height);
            return pick(gen);
        }
        float random(int range) {
            std::uniform_int_distribution<float> pick(-range, range);
            return pick(gen);
        }
        float random(int min, int max) {
            std::uniform_int_distribution<float> pick(-min, max);
            return pick(gen);
        }

        //
        float get(int x, int y) {
            const int w = texture.width  - 1;
            const int h = texture.height - 1;
            if (x < 0 || x > w || y < 0 || y > h) return -1;
            return data[y][x];
        }
        void set(int x, int y, float v) {
            const int w = texture.width  - 1;
            const int h = texture.height - 1;
            if (x < 0 || x > w || y < 0 || y > h) { return; }
            data[y][x] = v;
            if (data[y][x] > -1.0f) info.minval = std::min(info.minval, data[y][x]);
            info.maxval = std::max(info.maxval, data[y][x]);
        }
        
        void diamond(int x, int y, int size, float offset) {
            float len = 0.0f; float avg = 0.0f;
            float t = get(x, y - size); // top
            float r = get(x + size, y); // right
            float b = get(x, y + size); // bottom
            float l = get(x - size, y); // left
            if (t > 0) { avg += t; len++; }  
            if (r > 0) { avg += r; len++; }  
            if (b > 0) { avg += b; len++; }  
            if (l > 0) { avg += l; len++; }  
            avg /= 4;//(len > 0) ? len : 1;
            set(x, y, avg + offset);// % options.height;
        }

        void square(int x, int y, int size, float offset) {
            float len = 0.0f; float avg = 0.0f;
            float ul = get(x - size, y - size); // upper left
            float ur = get(x + size, y - size); // upper right
            float lb = get(x + size, y + size); // lower bottom
            float ll = get(x - size, y + size); // lower left
            if (ul > 0) { avg += ul; len++; }  
            if (ur > 0) { avg += ur; len++; }  
            if (lb > 0) { avg += lb; len++; }  
            if (ll > 0) { avg += ll; len++; }  
            avg /= 4;//(len > 0) ? len : 1;
            set(x, y, avg + offset);// % options.height;
            
        }

        void step(int size) {
            info.steps++;
            const int w = texture.width - 1;
            const int h = texture.height - 1;
            int x, y, half = size / 2;
            float scale = options.roughness * (float)size;
            if ((float)size / 2.0f < 1.0f) return;

            for (y = half; y < h; y += size) {
                for (x = half; x < w; x += size) {
                    square(x, y, half, random() * scale * 2 - scale);
                }
            }

            for (y = 0; y <= h; y += half) {
                for (x = (y + half) % size; x <= w; x += size) {
                    diamond(x, y, half, random() * scale * 2 - scale);
                }
            }   

            step(half);
        }
    public:
        char seed[32];
        const int seed_size = 32;
        struct Options {
            float height;
            float roughness;
            // int size;
        } options;
        struct Info {
            int steps;
            float maxval, minval;
            std::clock_t time;
        } info;
        struct Color {
            ImVec4 data;
            int range;

            Color() {
                range = 0;
                data = {
                    (float)(std::rand() % 256) / 255.0f,
                    (float)(std::rand() % 256) / 255.0f,
                    (float)(std::rand() % 256) / 255.0f,
                    255
                };
            };
        };
        std::vector<Color> colors;

        Terrain() {
            texture.width  = 512;
            texture.height = 512;

            // options
            options.height = 256.0f;
            options.roughness = 50.0f;

            // init 2d vector
            data.resize(texture.height);
            for (int y = 0; y < texture.height; y++) {
                data[y].resize(texture.width);
                for (int x = 0; x < texture.width; x++) {
                    data[y][x] = 0;
                }
            }
        };

        void term() {
            if (texture.data) SDL_DestroyTexture(texture.data);
        }

        SDL_Texture* get_texture() { return texture.data; };
        void set_texture(SDL_Texture* texture) { this->texture.data = texture; };

        void set_width(int width) { this->texture.width = width; };
        void set_height(int height) { this->texture.height = height; };
        int get_width() { return texture.width; };
        int get_height() { return texture.height; };
        
        void generate() {
            log("seed: ", seed);
            info.steps = 0;
            gen.seed(hasher((std::strlen(seed) == 0) ? random_string(seed_size) : seed));

            std::clock_t time_start = std::clock();
            // diamond square start
            int w = texture.width - 1;
            int h = texture.height - 1;
            set(0, 0, random());
            set(w, 0, random());
            set(0, h, random());
            set(w, h, random());
            step(texture.width);

            std::clock_t time_end = std::clock();
            info.time = time_end - time_start;
            log("minval: ", info.minval);
            log("maxval: ", info.maxval);
            log("steps: ", info.steps);
            log("time passed: ", ((float)info.time) / CLOCKS_PER_SEC);

            SDL_SetRenderTarget(render.data, texture.data);
            for (int y = 0; y < texture.height; y++) {
                for (int x = 0; x < texture.width; x++) {
                    Uint8 h = (Uint8)std::clamp((int)(data[y][x] / info.maxval * 255), 0, 255);
                    SDL_Color c;
                    if (colors.size() > 0) {
                        c = { 0, 0, 0, 255 };
                        for (auto col: colors) {
                            if (col.range < h) {
                                c = {
                                    (Uint8)(col.data.x * 255.0f),
                                    (Uint8)(col.data.y * 255.0f),
                                    (Uint8)(col.data.z * 255.0f),
                                    255
                                };
                            }
                        }
                    } else c = { h, h, h, 255 };
                    
                    render.pixel(x, y, c);
                }
            }
            SDL_SetRenderTarget(render.data, NULL);
        }
        void new_seed() {
            snprintf(seed, seed_size, "%s", random_string(seed_size).c_str());
        }
} terrain;

bool init(std::string title) {
    log(title);
    running = true;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { error(SDL_GetError()); return true; }

    Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    window.data = SDL_CreateWindow(title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window.width, window.height, window_flags);
    if (!window.data) { error(SDL_GetError()); return true; }

    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE;
    render.data = SDL_CreateRenderer(window.data, -1, renderer_flags);
    if (!render.data) { error(SDL_GetError()); return true; }

    Uint32 texture_format = SDL_GetWindowPixelFormat(window.data);
    terrain.set_texture(SDL_CreateTexture(render.data, texture_format, SDL_TEXTUREACCESS_TARGET, terrain.get_width(), terrain.get_height()));
    if (!terrain.get_texture()) { error(SDL_GetError()); return true; }

    return false;
}


void term() {
    terrain.term();
    if (render.data) SDL_DestroyRenderer(render.data);
    if (window.data) SDL_DestroyWindow(window.data);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    if (init("Diamond Square Demo")) { error("Cant' init!"); term(); return 1; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window.data, render.data);
    ImGui_ImplSDLRenderer_Init(render.data);

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            switch (e.type) {
                case SDL_QUIT: running = false; break;
                case SDL_WINDOWEVENT: {
                    if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                        running = false;
                    }
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                        SDL_GetWindowSize(window.data, &window.width, &window.height);
                    }
                } break;
            }
        }

        // imgui stuff
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        {
            // ImGui::ShowDemoWindow();

            ImGui::Begin("Diamond Square Demo");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Separator();
            ImGui::Text("Options:");
            // ImGui::InputInt("Size", terrain.get_width());
            ImGui::InputText("Seed", terrain.seed, terrain.seed_size); ImGui::SameLine();
            if (ImGui::Button("New")) { terrain.new_seed(); }
            ImGui::SliderFloat("Height", &terrain.options.height, 1.0f, 512.0f);
            ImGui::SliderFloat("Roughness", &terrain.options.roughness, 0.0f, 100.0f);
            if (ImGui::Button("Proceed")) { terrain.generate(); }
            ImGui::Text("Colors:");
            ImGui::Separator();
            if (ImGui::Button("Add new color range")) {
                if (terrain.colors.size() < 16) {
                    Terrain::Color c;
                    terrain.colors.push_back(c);
                }
            }
            for (int i = 0; i < terrain.colors.size(); i++) {
                ImGui::PushID(i);
                auto *color = &terrain.colors[i];
                // ImColor imc(color.data.r, color.data.g, color.data.b);
                std::string desc_id = "Color " + std::to_string(i);
                ImGui::Text("%02i.", i+1); ImGui::SameLine();
                if (ImGui::InputInt("##", &color->range)) {
                    color->range = std::clamp(color->range, 0, 255);
                } ImGui::SameLine();
                ImGui::ColorEdit3(desc_id.c_str(), (float*)&color->data, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                ImGui::SameLine();
                if (ImGui::Button("Del")) {
                    terrain.colors.erase(terrain.colors.begin() + i);
                    i--;
                }
                ImGui::PopID();
            }
            ImGui::End();
        }
        ImGui::Render();

        render.clear((SDL_Color){255, 255, 255});

        SDL_Rect tex_rect = { 0, 0, terrain.get_width(), terrain.get_height() };
        SDL_RenderCopy(render.data, terrain.get_texture(), NULL, &tex_rect);

        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(render.data);
    }

    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    term();
    return 0;
}
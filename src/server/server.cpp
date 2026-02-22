#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <boost/thread.hpp>
#include <SDL/SDL.h>
#undef main // SDL defines its own main and wants us to use SDL_main instead. NEIN NEIN!!!
#include "server/udp_server.hpp"

typedef struct {
    int x0, y0;
    int chunk_height;
    int width;
    int height;
    float zoom;
    float min_x, min_y;
    int max_iterations;
    int anti_aliasing;
} ImageChunk;

typedef struct{
    int x, y;
    float red;
    float green;
    float blue;
} Pixel;

std::deque<ImageChunk> to_generate;
std::mutex to_display_mutex;
std::queue<Pixel> to_display;

void handle_submit_work_packet(const std::unique_ptr<Buffer>& data) {
    std::lock_guard<std::mutex> lock(to_display_mutex);
    int size = data->read<int>();
    std::vector<Pixel> pixels;
    for (int i = 0; i < size; i++) {
        Pixel pixel;
        pixel.x = data->read<int>();
        pixel.y = data->read<int>();
        pixel.red = data->read<float>();
        pixel.green = data->read<float>();
        pixel.blue = data->read<float>();
        to_display.push(pixel);
    }
}

void request_work(const std::shared_ptr<udp_server>& server, const udp::endpoint& endpoint) {
    Buffer buffer;
    buffer.write((char)0x00);
    if (to_generate.empty()) {
        buffer.write(false);
    } else {
        buffer.write(true);
        auto image_chunk = to_generate.front(); to_generate.pop_front();
        buffer.write(image_chunk.x0);
        buffer.write(image_chunk.y0);
        buffer.write(image_chunk.chunk_height);
        buffer.write(image_chunk.width);
        buffer.write(image_chunk.height);
        buffer.write(image_chunk.zoom);
        buffer.write(image_chunk.min_x);
        buffer.write(image_chunk.min_y);
        buffer.write(image_chunk.max_iterations);
        buffer.write(image_chunk.anti_aliasing);
    }
    server->send_to(buffer, endpoint);
}

inline void SetPixel(SDL_Surface* screen, short x, short y, unsigned char red, unsigned char green, unsigned char blue) {
    unsigned char* pixel = &(((unsigned char*)(screen->pixels))[x*3+y*screen->pitch+0]);
    *pixel++ = red;
    *pixel++ = green;
    *pixel = blue;
} // SetPixel

void server_main(const std::shared_ptr<udp_server>& server) {
    server->set_read_callback([](std::unique_ptr<Buffer>& data, const udp::endpoint& sender, const std::shared_ptr<udp_server>& server) {
        printf("[debug] Packet from %s:%d\n", sender.address().to_string().c_str(), sender.port());
        if (data->remaining() < sizeof(char)) {
            printf("[error] Packet too small\n");
            return;
        }
        auto id = data->read<char>();
        printf("[debug] ID: %d\n", id);
        switch (id) {
            case 0x00: // Client requesting work
                printf("[info] Client requesting work\n");
                request_work(server, sender);
                break;
            case 0x01: // Client submitting work
                printf("[info] Client submitting work\n");
                handle_submit_work_packet(data);
                break;
            default:
                printf("[error] Unknown packet ID: %d\n", id);
                break;
        }
        if (data->remaining() > 0) {
            printf("[warn] Packet has %zu bytes left\n", data->remaining());
        }
    });
    try {
        server->start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
}

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("[error] Usage: %s <width> <height> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int width = std::stoi(argv[1]);
    int height = std::stoi(argv[2]);
    int port = std::stoi(argv[3]);
    printf("[info] Starting server with width: %d, height: %d, port: %d\n", width, height, port);
    printf("[info] Controls:\n");
    printf("[info] move: cursor keys\n");
    printf("[info] zoom: page up/down\n");
    printf("[info] anti-aliasing no/off: return\n");
    printf("[info] in-/decrease iterations: +/-\n");

    auto server = std::make_shared<udp_server>(Ipv::V4, port);
    boost::thread server_thread(server_main, server);

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("[error] SDL_Init failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Surface* screen = SDL_SetVideoMode(width, height, 24, SDL_SWSURFACE);
    if(screen == nullptr) {
        printf("[error] SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    bool need_to_redraw = true;
    bool running = true;
    float zoom = 2.0;
    float min_x = -1.50;
    float min_y = -1.0;
    int max_iterations = 100;
    int anti_aliasing = 0;
    while(running) {
        if (server_thread.timed_join(boost::posix_time::seconds(0))) {
            printf("[info] Server thread exited\n");
            break;
        }
        if(need_to_redraw) {
            const int chunk_height = 10;
            to_generate.clear();
            for (int i = 0; i < height; i += chunk_height) {
                to_generate.push_back({
                    0, i,
                    chunk_height,
                    width, height,
                    zoom,
                    min_x, min_y,
                    max_iterations,
                    anti_aliasing
                });
            }
            need_to_redraw = false;
        }
        if (!to_display.empty()) {
            std::lock_guard<std::mutex> lock(to_display_mutex);
            while (!to_display.empty()) {
                auto pixel = to_display.front(); to_display.pop();
                SetPixel(screen, pixel.x, pixel.y, pixel.red, pixel.green, pixel.blue);
            }
            SDL_UpdateRect(screen, 0, 0, width, height);
        }
        SDL_Event event;
        if (SDL_PollEvent(&event) == 0) {
            continue;
        }
        switch (event.type) {
            case SDL_QUIT:
                printf("[info] SDL_QUIT event received\n");
                running = false;
                break;
            case SDL_KEYDOWN:
                //Handle key presses
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        min_y -= 0.1f * zoom;
                        need_to_redraw = true;
                        break;
                    case SDLK_DOWN:
                        min_y += 0.1f * zoom;
                        need_to_redraw = true;
                        break;
                    case SDLK_LEFT:
                        min_x -= 0.1f * zoom;
                        need_to_redraw = true;
                        break;
                    case SDLK_RIGHT:
                        min_x += 0.1f * zoom;
                        need_to_redraw = true;
                        break;
                    case SDLK_PAGEUP:
                    case SDLK_m:
                        zoom *= 1.2f;
                        min_x -= zoom * 0.1f;
                        min_y -= zoom * 0.1f;
                        need_to_redraw = true;
                        break;
                    case SDLK_PAGEDOWN:
                        case SDLK_p:
                        zoom /= 1.2f;
                        min_x += zoom * 0.1f;
                        min_y += zoom * 0.1f;
                        need_to_redraw = true;
                        break;
                    case SDLK_KP_PLUS:
                    case SDLK_PLUS:
                        max_iterations += 500;
                        need_to_redraw = true;
                        break;
                    case SDLK_KP_MINUS:
                    case SDLK_MINUS:
                        if (max_iterations > 500)
                            max_iterations -= 500;
                        need_to_redraw = true;
                        break;
                    case SDLK_RETURN:
                    case SDLK_q:
                        if(anti_aliasing == 0) anti_aliasing = 1;
                        else anti_aliasing = 0;
                        need_to_redraw = true;
                        break;
                    case SDLK_ESCAPE:
                        running = false;
                        break;

                }
                break;
            case SDL_VIDEORESIZE:
                width = event.resize.w;
                height = event.resize.h;
                need_to_redraw = true;
                break;
            default:
                break;
        }
    }
    SDL_FreeSurface(screen);
    SDL_Quit();
    server->stop();
    server_thread.join();
    return EXIT_SUCCESS;
}

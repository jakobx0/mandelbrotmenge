#include "client/udp_client.hpp"
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <algorithm>
#include <cmath>
#include <boost/thread.hpp>

typedef struct{
    int x, y;
    float red;
    float green;
    float blue;
} Pixel;

void create_request_work_packet(Buffer& buffer) {
    buffer.write((char)0x00);
}

void send_pixels(Buffer& buffer, const std::vector<Pixel>& pixels) {
    buffer.write((char)0x01);
    buffer.write((int)pixels.size());
    for (const auto& pixel : pixels) {
        buffer.write(pixel.x);
        buffer.write(pixel.y);
        buffer.write(pixel.red);
        buffer.write(pixel.green);
        buffer.write(pixel.blue);
    }
}

inline double sqr(double v)
{
    return v*v;
} // sqr

int calc_gray(float cx, float cy, int max_iter) {
    //erstmal auf 3 gesetzt -> wie vorgegeben
    const float max_square = 3.0;
    float square = 0;
    int iter = 0;
    float x, y, xt, yt;
    x = 0; y = 0;
    while((square <= max_square) && (iter < max_iter)) {
        xt = x*x - y*y + cx;
        yt = 2*x*y + cy;
        x = xt;
        y = yt;
        square = x*x + y*y;
        iter++;
    }
    return iter;
}

int HSVToRGB(float h, float s, float v, float& r, float& g, float& b)
{
    int i;
    float f, p, q, t;
    float red[2], green[2], blue[2];

    if(s == 0) {
        r = v; g = v; b = v;
        return 0;
    } // if
    else {
        if(h == 360) h = 0;

        i = (int)h/60;
        f = ((int)h % 60)/60.0;

        switch(i)	{
            case 0:
                red[0] = 255;
                green[0] = 0;
                blue[0] = 0;

                red[1] = 255;
                green[1] = 255;
                blue[1] = 0;
                break;

            case 1:
                red[0] = 255;
                green[0] = 255;
                blue[0] = 0;

                red[1] = 0;
                green[1] = 255;
                blue[1] = 0;
                break;

            case 2:
                red[0] = 0;
                green[0] = 255;
                blue[0] = 0;

                red[1] = 0;
                green[1] = 255;
                blue[1] = 255;
                break;

            case 3:
                red[0] = 0;
                green[0] = 255;
                blue[0] = 255;

                red[1] = 0;
                green[1] = 0;
                blue[1] = 255;
                break;

            case 4:
                red[0] = 0;
                green[0] = 0;
                blue[0] = 255;

                red[1] = 255;
                green[1] = 0;
                blue[1] = 255;
                break;

            case 5:
                red[0] = 255;
                green[0] = 0;
                blue[0] = 255;

                red[1] = 255;
                green[1] = 0;
                blue[1] = 0;
                break;
        } // case

        r = f*red[1] + (1-f)*red[0];
        g = f*green[1] + (1-f)*green[0];
        b = f*blue[1] + (1-f)*blue[0];

        r = s*r + (1-s)*255;
        g = s*g + (1-s)*255;
        b = s*b + (1-s)*255;

        r *= v;
        g *= v;
        b *= v;

        return 0;
    }
} // HSVToRGB

void calc_chunk(const std::shared_ptr<udp_client>& client, int x0, int y0, int chunk_height, int width, int height ,float min_x, float min_y, float zoom, int max_iterations, int anti_aliasing){
    std::vector<Pixel> pixels;
    const int max_chunk = 900;
    int pixel_count = 0;

    // calculation over min and max
    const float factor = zoom / std::min((float)width, (float)height);

    for (int y = y0; y < y0 + chunk_height; y++) {
        for (int x = x0; x < x0 + width; x++) {
            pixel_count = (pixel_count + 1) % max_chunk;

            float hue = 0.0;

            for(float dy = -anti_aliasing; dy <= anti_aliasing; dy++) {
                for(float dx = -anti_aliasing; dx <= anti_aliasing; dx++) {

                    float cx;
                    float cy;

                    cy = min_y + (y+(dy+(float)anti_aliasing)/(2.0*(float)anti_aliasing+1))*factor;
                    cx = min_x + (x+(dx+(float)anti_aliasing)/(2.0*(float)anti_aliasing+1))*factor;

                    hue += 360*log(calc_gray(cx, cy, max_iterations))/log(max_iterations);

                } // for
            } // for

            hue /= sqr(2.0*anti_aliasing+1);

            float value = 1.0;
            if (hue > 359) {
                value = 0.0;
            }

            float r, g, b;
            HSVToRGB(hue, 1.0, value, r, g, b);

            pixels.push_back({x, y, r, g, b});

            if (pixel_count == 0) {
                Buffer buffer;
                send_pixels(buffer, pixels);
                client->send(buffer);
                pixels.clear();
            }
        }
    }
    Buffer buffer;
    send_pixels(buffer, pixels);
    client->send(buffer);
}


std::mutex working;
int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    auto host = argv[1];
    auto port = argv[2];
    auto client = std::make_shared<udp_client>(host, port, Ipv::V4);
    client->set_read_callback([](std::unique_ptr<Buffer>& data, const udp::endpoint& sender, const std::shared_ptr<udp_client>& client) {
        printf("Packet from %s:%d\n", sender.address().to_string().c_str(), sender.port());

        if (data->remaining() < sizeof(char)) {
            printf("Packet too small\n");
            return;
        }
        auto id = data->read<char>();
            printf("ID: %d\n", id);
        if (id == 0x00) { // Got work from server
            if (!data->read<bool>()) {
                printf("Got no work from server\n");
            }else {
                std::lock_guard<std::mutex> lockGuard(working);
                printf("Got work from server\n");
                auto x0 = data->read<int>();
                auto y0 = data->read<int>();
                auto chunk_height = data->read<int>();
                auto width = data->read<int>();
                auto height = data->read<int>();
                auto zoom = data->read<float>();
                auto min_x = data->read<float>();
                auto min_y = data->read<float>();
                auto max_iterations = data->read<int>();
                auto anti_aliasing = data->read<int>();
                // Berechnen des chunks
                printf("Calculating chunk: x0=%d, y0=%d, chunk_height=%d, width=%d, height=%d, zoom=%f, min_x=%f, min_y=%f, max_iterations=%d, anti_aliasing=%d\n", x0, y0, chunk_height, width, height, zoom, min_x, min_y, max_iterations, anti_aliasing);
                calc_chunk(client, x0, y0, chunk_height, width, height, min_x, min_y, zoom, max_iterations, anti_aliasing);
            }
        } else {
            printf("Unknown packet ID: %d\n", id);
        }
        if(data->remaining() > 0) {
            printf("Packet has %zu bytes left\n", data->remaining());
        }
    });

    boost::thread work_thread([client]{
        while (client->alive()) {
            // std::cin.get(); // debugging
            if (working.try_lock()) {
                Buffer buffer;
                create_request_work_packet(buffer);
                client->send(buffer);
                working.unlock();
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
        }
    });
    try {
        client->start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    client->stop();
    return 0;
}

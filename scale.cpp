#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <cmath>

int DEFAULT_SPLATS;

std::random_device rd;
std::default_random_engine gen(rd());
std::uniform_real_distribution<float> uniform(0.0, 1.0);
inline float rng() { return uniform(gen); }

#include "bmp.cpp"
#include "splat.cpp"

int main(int argc, const char **argv){
	if(argc < 4){
		std::cerr << "Incorrect usage:\n";
		std::cerr << argv[0] << " <config.loc> <output.loc> <scale>\n";
		return 0;
	}

	int scale = std::stoi(argv[3]);

	read_state(argv[1]);

	if(argc >= 5){
		int _S = std::stoi(argv[4]);
		float S = S/1e2;
		
		for(int i=0; i<n; ++i){
			if(_S == -1){
				for(int z=0; z<3; ++z)
					splat[i].c[z] = rng();
				continue;
			}

			splat[i].r[0] *= S;
			splat[i].r[1] *= S;
		}
	}

	width *= scale, height *= scale;
	data2 = new unsigned char[width*height*3];
	canvas = new float[width*height*3];

	for(int i=0; i<n; ++i){
		splat[i].x *= scale, splat[i].y *= scale;
		splat[i].r[0] *= scale, splat[i].r[1] *= scale;
	}

	auto start = std::chrono::high_resolution_clock::now();

	paint();

	auto now = std::chrono::high_resolution_clock::now();
	float duration = ((std::chrono::duration<float>) (now-start)).count();
	std::cout << "upscaled in " << (int) std::floor(duration*1000) << "ms\n";

	write(argv[2]);
}

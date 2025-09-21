// HYPER

const int TIMELAPSE = 0;
const int DEFAULT_SPLATS = 256;
const int DEFAULT_PARTICLES = 32;

const float IMP_TUNE = 1.0;
const int IMP_RAD = 10;

// DFL

#include <algorithm>
#include <iostream>
#include <random>
#include <chrono>

#include "src/write_bmp.cpp"
#include "src/read_bmp.cpp"

// RESUME

#include <signal.h>

volatile sig_atomic_t signal_status = 0;

void sighandler(int s){
	signal_status = s;
}

// PRNG

namespace c0{
	std::random_device rd;
	std::default_random_engine gen(rd());
	std::uniform_real_distribution<float> uniform(0.0, 1.0);
	std::normal_distribution<float> gaussian(0.0, 1.0);
}

inline float rng(){
	return c0::uniform(c0::gen);
}

inline float norm(){
	return c0::gaussian(c0::gen);
}

// CORE

#include "splat.cpp"

// REFERENCE IMAGE

namespace c1{
	unsigned char *data;
	float *imp, imp_total;
};

void load_reference(const char *filename){
	auto start = std::chrono::high_resolution_clock::now();

	int w2, h2;
	read_bmp(filename, &c1::data, &w2, &h2);

	if((w2 != width || h2 != height) && !(width == -1 && height == -1)){
		std::cerr << "IMAGE DIMENSION MISMATCH: " << width << 'x' << height << " VS ";
		std::cerr << w2 << 'x' << h2 << "\nABORTING\n";
		exit(1);
	}

	width = w2, height = h2;

	diag = std::sqrt(width*width + height*height);

	data2 = new unsigned char[width*height*3];
	c1::imp = new float[width*height*3];
	canvas = new float[width*height*3];

	const int rad = IMP_RAD, crad = std::ceil(rad * 2.2);

	for(int y=0; y<height; ++y){
		for(int x=0; x<width; ++x){
			int xmin = std::max(0, x-crad),
				xmax = std::min(width-1, x+crad);
			for(int z=0; z<3; ++z){
				double sum = 0, all = 0;
				for(int X=xmin; X<=xmax; ++X){
					double rx = (double)(X-x) / rad;
					double F = exp(-rx*rx);
					sum += c1::data[3*(X+y*width)+z] * F;
					all += F;
				}
				data2[3*(x+y*width)+z] = sum/all;
			}
		}
	}

	float *col = new float[height*3];

	for(int x=0; x<width; ++x){
		for(int y=0; y<height; ++y){
			int ymin = std::max(0, y-crad),
				ymax = std::min(height-1, y+crad);
			for(int z=0; z<3; ++z){
				double sum = 0, all = 0;
				for(int Y=ymin; Y<=ymax; ++Y){
					double ry = (double)(Y-y) / rad;
					double F = exp(-ry*ry);
					sum += data2[3*(x+Y*width)+z] * F;
					all += F;
				}
				col[y*3+z] = sum/all;
			}
		}
		for(int y=0; y<height; ++y){
			for(int z=0; z<3; ++z){
				data2[3*(x+y*width)+z] = col[y*3+z];
			}
		}
	}

	delete[] col;

	for(int y=0; y<height; ++y){
		for(int x=0; x<width; ++x){
			for(int z=0; z<3; ++z){
				float F = std::abs(data2[3*(x+y*width)+z] - c1::data[3*(x+y*width)+z]) * IMP_TUNE + 1.0;
				c1::imp[3*(x+y*width)+z] = F; c1::imp_total += F;
			}
		}
	}

	auto now = std::chrono::high_resolution_clock::now();
	float duration = ((std::chrono::duration<float>) (now - start)).count();
	std::cout << "loaded " << std::floor(duration*1e3) << "ms\n";
}

// LOSS IMPL

float loss(_state &s){
	paint(s);

	long long ret = 0;

	for(int i=0; i<width*height*3; ++i){
		int diff = (int) c1::data[i] - std::max(0, std::min(255, (int) std::floor(255.0 * canvas[i])));
		ret += diff*diff * c1::imp[i];
	}

	return s.loss = (float) ret/c1::imp_total;
}

// MAIN

int main(){
	srand(time(NULL));

	int E = read_state("scene.conf");

	load_reference("reference.bmp");

	if(E) init_particles();

	/*
	std::cout << best.data[0].x << ' ' << best.data[1].y << '\n';
	std::cout << best.data[0].r[0] << ' ' << best.data[1].r[1] << '\n';
	paint(best); write("progress.bmp"); return 1;
	*/

	signal(SIGINT, sighandler);

	float a1 = 0, a2 = 0; int y = 1, y2 = 0, count = 0; while(true){
		if(signal_status == SIGINT){
			std::cout << "\nterminated";
			break;
		}

		auto start = std::chrono::high_resolution_clock::now();

		// BEGIN UPDATE

		for(int i=0; i<particles; ++i){
			for(int j=0; j<n; ++j){
#define S particle[i].now.data[j]
#define B particle[i].best.data[j]
#define G best.data[j]
				auto E = [] (float a, float b) { return (a+b)/2.0 + norm() * std::abs(a-b); };

				S.x = std::clamp(E(B.x, G.x), 0.0, (double) width);
				S.y = std::clamp(E(B.y, G.y), 0.0, (double) height);

				S.r[0] = std::clamp(E(B.r[0], G.r[0]), 0.5, diag/16.0);
				S.r[1] = std::clamp(E(B.r[1], G.r[1]), 0.5, diag/16.0);

				S.r[4] = std::fmod(std::fmod(E(B.r[4], G.r[4]), M_PI*2.0) + M_PI*2.0, M_PI*2.0);
				S.r[2] = std::cos(S.r[2]);
				S.r[3] = std::cos(S.r[2]);

				for(int z=0; z<3; ++z)
					S.c[z] = std::clamp(E(B.c[z], G.c[z]), 0.0, 1.0);
#undef S
#undef B
#undef G
			}

			if(loss(particle[i].now) < particle[i].best.loss)
				copystate(particle[i].now, particle[i].best);

			if(particle[i].best.loss < best.loss)
				copystate(particle[i].best, best);
		}

		// END UPDATE

		auto now = std::chrono::high_resolution_clock::now();
		float duration = ((std::chrono::duration<float>) (now - start)).count();
		a1 += duration, a2 += duration; ++y2;

		if(iter%1000 == 0 && iter) std::cout << '\n';

		if(a1 > 0.2){
			std::cout << best.loss << '\n';
			for(int i=0; i<particles; ++i)
				std::cout << particle[i].best.loss << ' ' << particle[i].now.loss << '\n';
			std::cout << '\n';
			/*
			std::cout << '\r' << iter/1000 << "K  " << best.loss << "E  ";
			std::cout << (int) std::floor(a1/y2*1000) << "T  ";
			//std::cout << hist::avg(error) << "A  " << RATE << "R  ";
			std::flush(std::cout);
			*/
			a1 = 0, y2 = 0;
		}

		if(TIMELAPSE && iter%1000 == 0){
			paint(best);

			{
				std::string s = "frames/" + std::to_string(iter/1000) + ".bmp";
				write(s.c_str());
			};
			{
				std::string s = "frames/" + std::to_string(iter/1000) + ".conf";
				write_state(s.c_str());
			};
			{
				std::string s = "frames/" + std::to_string(iter/1000) + ".stat";
				std::ofstream v(s);
				v << best.loss << '\n';
				v.close();
			};
		}

		if(a2 > 1) y = 1, a2 = 0;
		if(y) paint(best), write("progress.bmp"), y = 0;

		++iter;
	}

	std::cout << '\n';

	write_state("scene.conf");
}

// HYPER

const int RECORD = 1;
const int DEFAULT_SPLATS = 4096;
const int DEFAULT_PARTICLES = 32;

const float IMP_TUNE = 2.0;
const int IMP_RAD = 10;

// DFL

#include <algorithm>
#include <iostream>
#include <random>
#include <chrono>

#include "bmp.cpp"

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
}

inline float rng(){
	return c0::uniform(c0::gen);
}

// CORE

#include "splat.cpp"

// LEARNING RATES

namespace hist{
	float sum[3] = {0, 0, 0};
	int count[3] = {0, 0, 0};

	void tick(int i, float v){
		sum[i] += v * 1e3; ++count[i];
		if(count[i] > 500) count[i] /= 2, sum[i] /= 2;
	}

	float avg(int i){
		return count[i] ? sum[i] / count[i] : 0.0;
	}
};

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

float loss(){
	paint();

	long long ret = 0;

	for(int i=0; i<width*height*3; ++i){
		int diff = (int) c1::data[i] - std::max(0, std::min(255, (int) std::floor(255.0 * canvas[i])));
		ret += diff*diff * c1::imp[i];
	}

	return (float) ret/c1::imp_total;
}

// MAIN

int main(){
	srand(time(NULL));

	int E = read_state("scene.conf");

	load_reference("reference.bmp");

	if(E) init_state();

	float error = loss();

	signal(SIGINT, sighandler);

	float a1 = 0, a2 = 0; int y = 1, y2 = 0, count = 0; while(true){
		if(signal_status == SIGINT){
			std::cout << "\nterminated";
			break;
		}

		auto start = std::chrono::high_resolution_clock::now();

		// BEGIN UPDATE
		
		int T = rand()%3, i = rand()%n;

		_splat old = splat[i];

		if(T == 0){
			splat[i].x = rng()*width;
			splat[i].y = rng()*height;

			splat[i].r[0] = rng()*diag/16 + 3.0;
			splat[i].r[1] = rng()*diag/16 + 3.0;

			splat[i].r[4] = rng()*M_PI*2.0;
			splat[i].r[2] = std::cos(splat[i].r[4]);
			splat[i].r[3] = std::sin(splat[i].r[4]);

			for(int z=0; z<4; ++z) splat[i].c[z] = rng();
		}

		if(T == 1){
			float RATE = rng()*0.1;

			auto M = [RATE] (float &d, float m) {
				d = std::max(0.0, std::min(1.0, d/m + (rng()*2.0-1.0)*RATE)) * m;
			};

			M(splat[i].x, width), M(splat[i].y, height);

			splat[i].r[0] = std::max(0.5, std::min(
				diag/16.0, splat[i].r[0] + (rng()*2.0-1.0)*RATE*16/diag));
			splat[i].r[1] = std::max(0.5, std::min(
				diag/16.0, splat[i].r[1] + (rng()*2.0-1.0)*RATE*16/diag));

			for(int z=0; z<4; ++z) M(splat[i].c[z], 1.0);

			float E = splat[i].r[1]>splat[i].r[0] ?
				splat[i].r[1]/splat[i].r[0] : splat[i].r[0]/splat[i].r[1];
			float R2 = RATE + (1.0-RATE) * exp(1.0-E);
			splat[i].r[4] = std::fmod(splat[i].r[4]/M_PI/2.0 + (rng()*2.0-1.0)*R2, 1.0)*M_PI*2.0;
			splat[i].r[2] = std::cos(splat[i].r[4]);
			splat[i].r[3] = std::sin(splat[i].r[4]);
		}

		if(T == 2){
			float RATE = rng()*0.5;
			int C = rand()%9;

			auto M = [RATE] (float &d, float m) {
				d = std::max(0.0, std::min(1.0, d/m + (rng()*2.0-1.0)*RATE)) * m;
			};

			if(C == 0) M(splat[i].x, width);
			if(C == 1) M(splat[i].y, height);

			if(C == 2) splat[i].r[0] = std::max(0.5, std::min(
				diag/16.0, splat[i].r[0] + (rng()*2.0-1.0)*RATE*16/diag));
			if(C == 3) splat[i].r[1] = std::max(0.5, std::min(
				diag/16.0, splat[i].r[1] + (rng()*2.0-1.0)*RATE*16/diag));

			if(C == 4){
				float E = splat[i].r[1]>splat[i].r[0] ?
					splat[i].r[1]/splat[i].r[0] : splat[i].r[0]/splat[i].r[1];
				float R2 = RATE + (1.0-RATE) * exp(1.0-E);
				splat[i].r[4] = std::fmod(splat[i].r[4]/M_PI/2.0 + (rng()*2.0-1.0)*R2, 1.0)*M_PI*2.0;
				splat[i].r[2] = std::cos(splat[i].r[4]);
				splat[i].r[3] = std::sin(splat[i].r[4]);
			}

			for(int z=0; z<4; ++z) if(C == 5+z) M(splat[i].c[z], 1.0);
		}

		int reject = 0;
		float new_error = loss();
		if(new_error > error) splat[i] = old, hist::tick(T, 0), reject = 1;
		else hist::tick(T, error-new_error), error = new_error;

		// END UPDATE

		auto now = std::chrono::high_resolution_clock::now();
		float duration = ((std::chrono::duration<float>) (now - start)).count();
		a1 += duration, a2 += duration; ++y2;

		if(iter%1000 == 0 && iter) std::cout << '\n';

		if(a1 > 0.5){
			std::cout << '\r' << iter/1000 << "K  " << error << "E  ";
			std::cout << hist::avg(0) << ' ' << hist::avg(1) << ' ' << hist::avg(2) << "A  ";
			std::cout << (int) std::floor(a1/y2*1000) << "T  ";
			std::flush(std::cout);
			a1 = 0, y2 = 0;
		}

		if(RECORD && iter%1000 == 0){
			if(reject) paint(), reject = 0;

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
				v << error << ',' << hist::avg(0) << ',' << hist::avg(1) << ',' << hist::avg(2) << '\n';
				v.close();
			};
		}

		if(a2 > 1) y = 1, a2 = 0;
		if(y){
			if(reject) paint(), reject = 0;
			write("progress.bmp"), y = 0;
		}

		// AUTO PARAMETERIZE
		{
			if(iter == 1600e3) break;
		};

		++iter;
	}

	std::cout << '\n';

	write_state("scene.conf");
}

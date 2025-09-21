const int TIMELAPSE = 1;
const int DEFAULT_SPLATS = 512;
const int SPRAY = 0;
const int MOVE_ALL = 0;
const int MOVE_FAST = 1;
const float RATE = 0.1;
const float BACK = 0.0;

#include <iostream>
#include <random>
#include <chrono>

#include <signal.h>

volatile sig_atomic_t signal_status = 0;

void sighandler(int s){
	signal_status = s;
}

#include "src/write_bmp.cpp"
#include "src/read_bmp.cpp"

std::random_device rd;
std::default_random_engine gen(rd());
std::uniform_real_distribution<float> uniform(0.0, 1.0);

inline float rng(){
	return uniform(gen);
}

namespace hist{
	float last_error;
	int count = 0;

	void tick(float e){
		++count;
		if(count == 1000){
			last_error = (e+last_error)/2;
			count /= 2;
		}
	}

	float avg(float e){
		return count ? (last_error-e)/count : 0.0;
	}
};

#include "splat.cpp"

unsigned char *data;

double loss(){
	int ret = 0;

	for(int i=0; i<width*height*3; ++i){
		int diff = (int) data[i] - std::max(0, std::min(255, (int) std::floor(255.0 * canvas[i])));
		ret += diff*diff;
	}

	return (double) ret/ width / height / 3.0;
}

int main(){
	srand(time(NULL));

	int E = read_splats("scene.conf");

	read_bmp("reference.bmp", &data, &width, &height);
	float diag = std::sqrt(width*width + height*height);
	data2 = new unsigned char[width*height*3];
	canvas = new float[width*height*3];
	
	if(E){
		n = DEFAULT_SPLATS;
		splat = new _splat[n];

		for(int i=0; i<n; ++i){
			splat[i].x = rng()*width, splat[i].y = rng()*height;
			splat[i].r[0] = rng()*diag/16 + 3.0;
			splat[i].r[1] = rng()*diag/16 + 3.0;
			splat[i].r[4] = rng()*M_PI*2.0;
			splat[i].r[2] = std::cos(splat[i].r[4]);
			splat[i].r[3] = std::sin(splat[i].r[4]);
			for(int j=0; j<4; ++j) splat[i].c[j] = rng();
		}
	}

	paint();
	double error = loss();
	hist::last_error = error;

	signal(SIGINT, sighandler);

	float a1 = 0, a2 = 0; int y = 1, count = 0; while(true){
		if(signal_status == SIGINT){
			std::cout << "\nterminated";
			break;
		}

		auto start = std::chrono::high_resolution_clock::now();

		int i = rand()%n, j = i;
		if(i != n-1) j = i+rand()%(n-1-i);

		_splat o1 = splat[i], o2 = splat[j];

		auto M = [] (float &d, float m) {
			d = std::max(0.0, std::min(1.0, d/m + (rng()*2.0-1.0)*RATE)) * m;
		};

		int C;

		if(MOVE_ALL) C = (1<<9) - 1;
		else if(MOVE_FAST) C = 1 + (rand()%((1<<9)-1));
		else C = 1 << (rand()%9);

		if(C & (1<<0)) M(splat[i].x, width);
		if(C & (1<<1)) M(splat[i].y, height);

		if(C & (1<<2)) splat[i].r[0] = std::max(0.5, std::min(
			diag/16.0, splat[i].r[0] + (rng()*2.0-1.0)*RATE*16/diag));
		if(C & (1<<3)) splat[i].r[1] = std::max(0.5, std::min(
			diag/16.0, splat[i].r[1] + (rng()*2.0-1.0)*RATE*16/diag));

		for(int z=0; z<4; ++z) if(C & (1<<(4+z))) M(splat[i].c[z], 1.0);

		if(C & (1<<8)){
			float E = splat[i].r[1]>splat[i].r[0] ? splat[i].r[1]/splat[i].r[0] : splat[i].r[0]/splat[i].r[1];
			float R2 = RATE + (1.0-RATE) * exp(1.0-E);
			splat[i].r[4] = std::fmod(splat[i].r[4]/M_PI/2.0 + (rng()*2.0-1.0)*R2, 1.0)*M_PI*2.0;
			splat[i].r[2] = std::cos(splat[i].r[4]);
			splat[i].r[3] = std::sin(splat[i].r[4]);
		}

		if(SPRAY){
			splat[i].x = rng()*width, splat[i].y = rng()*height;
			splat[i].r[0] = rng()*diag/16 + 0.5;
			splat[i].r[1] = rng()*diag/16 + 0.5;
			splat[i].r[4] = rng()*M_PI*2.0;
			splat[i].r[2] = std::cos(splat[i].r[4]);
			splat[i].r[3] = std::sin(splat[i].r[4]);
			for(int j=0; j<4; ++j) splat[i].c[j] = rng();
		}

		if(i != j && splat[j].x*splat[j].y > splat[i].x*splat[i].y) std::swap(splat[i], splat[j]);

		paint();
		double new_error = loss();

		bool reject = 0;
		if(new_error > error + BACK) splat[i] = o1, splat[j] = o2, reject = 1;
		else error = new_error;

		hist::tick(error);

		auto now = std::chrono::high_resolution_clock::now();
		float duration = ((std::chrono::duration<float>) (now - start)).count();
		a1 += duration, a2 += duration;

		if(iter%1000 == 0) std::cout << '\n';

		if(a1 > 0.2){
			std::cout << '\r' << iter/1000 << "K  " << error << "E  " << (int) std::floor(duration*1000) << "T  ";
			std::cout << hist::avg(error) << "A  " << RATE << "R  ";
			std::flush(std::cout);
			a1 = 0;
		}

		if(TIMELAPSE && iter%1000 == 0){
			if(reject) paint();

			{
				std::string s = "frames/" + std::to_string(iter/1000) + ".bmp";
				write(s.c_str());
			};
			{
				std::string s = "frames/" + std::to_string(iter/1000) + ".conf";
				write_splats(s.c_str());
			};
			{
				std::string s = "frames/" + std::to_string(iter/1000) + ".stat";
				std::ofstream v(s);
				v << error << '\n';
				v.close();
			};
		}

		if(a2 > 1) y = 1, a2 = 0;
		if(!reject && y) write("progress.bmp"), y = 0;

		++iter;
	}

	std::cout << '\n';

	write_splats("scene.conf");
}

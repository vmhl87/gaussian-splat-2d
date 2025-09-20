const int TIMELAPSE = 1;
const int DEFAULT_SPLATS = 512;
const double RATE = 0.1;

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
std::uniform_real_distribution<double> uniform(0.0, 1.0);

inline double rng(){
	return uniform(gen);
}

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
	int E = read_splats("scene.conf");

	read_bmp("reference.bmp", &data, &width, &height);
	int diag = std::sqrt(width*width + height*height);
	data2 = new unsigned char[width*height*3];
	canvas = new double[width*height*3];
	
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

	signal(SIGINT, sighandler);

	bool y = 1; while(true){
		if(signal_status == SIGINT){
			std::cout << "\nterminated";
			break;
		}

		auto start = std::chrono::high_resolution_clock::now();

		int i = rand()%n, j = i;
		if(i != n-1) j = i+rand()%(n-1-i);

		_splat o1 = splat[i], o2 = splat[j];

		auto M = [] (double &d, double m) {
			d = std::max(0.0, std::min(1.0, d/m + (rng()*2.0-1.0)*RATE)) * m;
		};

		M(splat[i].x, width);
		M(splat[i].y, height);
		M(splat[i].r[0], diag/16);
		M(splat[i].r[1], diag/16);
		for(int z=0; z<4; ++z) M(splat[i].c[z], 1.0);
		splat[i].r[4] = std::fmod(splat[i].r[4]/M_PI/2.0 + (rng()*2.0-1.0)*RATE, 1.0)*M_PI*2.0;
		splat[i].r[2] = std::cos(splat[i].r[4]);
		splat[i].r[3] = std::sin(splat[i].r[4]);

		if(i != j && splat[j].x*splat[j].y > splat[i].x*splat[i].y) std::swap(splat[i], splat[j]);

		paint();
		double new_error = loss();

		bool reject = 0;
		if(new_error > error) splat[i] = o1, splat[j] = o2, reject = 1;
		else error = new_error;

		auto now = std::chrono::high_resolution_clock::now();
		double duration = ((std::chrono::duration<double>) (now - start)).count();
		std::cout << '\r' << iter/1000 << "K  " << error << "E  " << (int) std::floor(duration*1000) << "T  ";
		std::flush(std::cout);

		if(TIMELAPSE && iter%1000 == 0){
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

		if((++iter)%30 == 0) y = 1;
		if(!reject && y) write("progress.bmp"), y = 0;
	}

	std::cout << '\n';

	write_splats("scene.conf");
}

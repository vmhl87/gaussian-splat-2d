#include <iostream>
#include <random>
#include <thread>
#include <chrono>

#include <signal.h>

volatile sig_atomic_t signal_status = 0;

void sighandler(int s){
	signal_status = s;
}

#include "write_bmp.cpp"
#include "read_bmp.cpp"

std::random_device rd;
std::default_random_engine gen(rd());
std::uniform_real_distribution<double> uniform(0.0, 1.0);

inline double rng(){
	return uniform(gen);
}

struct _splat{
	double x, y, r[4], c[4];
};

const int n = 1000;
_splat splat[n];
int iter = 0;

#include "serialize.cpp"

unsigned char *data, *data2;
int width, height;
double *canvas;

void _paint(int xmin, int ymin, int xmax, int ymax){
	for(int i=0; i<n; ++i){

		double R = std::max(splat[i].r[0], splat[i].r[1]);

		int Xmin = std::max(xmin,
				(int) std::floor(splat[i].x-R*3.0)),
			Xmax = std::min(xmax,
				(int) std::ceil(splat[i].x+R*3.0)),
			Ymin = std::max(ymin,
				(int) std::floor(splat[i].y-R*3.0)),
			Ymax = std::min(ymax,
				(int) std::ceil(splat[i].y+R*3.0));

		for(int y=Ymin; y<Ymax; ++y){
			for(int x=Xmin; x<Xmax; ++x){
				int j = 3*(x+y*width);

				double rx = splat[i].x-x, ry = splat[i].y-y;
				double vx = (rx*splat[i].r[2]+ry*splat[i].r[3])/splat[i].r[0],
					   vy = (ry*splat[i].r[2]-rx*splat[i].r[3])/splat[i].r[1];
				double alpha = exp(-vx*vx-vy*vy) * splat[i].c[3];

				for(int z=0; z<3; ++z)
					canvas[j+z] = canvas[j+z]*(1.0-alpha) + splat[i].c[z]*alpha;
			}
		}
	}
}

void paint(){
	for(int i=0; i<width*height*3; ++i) canvas[i] = 1.0;
	
	int nthreads = 10;
	std::thread threads[nthreads-1];

	for(int i=1; i<nthreads; ++i)
		threads[i-1] = std::thread(_paint, 0, height*(i-1)/nthreads, width, height*i/nthreads);

	_paint(0, height*(nthreads-1)/nthreads, width, height);
	
	for(int i=1; i<nthreads; ++i) threads[i-1].join();
}

double loss(){
	int ret = 0;

	for(int i=0; i<width*height*3; ++i){
		int diff = (int) data[i] - std::max(0, std::min(255, (int) std::floor(255.0 * canvas[i])));
		ret += diff*diff;
	}

	return (double) ret * 100.0 / width / height / 3.0 / 255.0 / 255.0;
}

void write(const char *filename){
	for(int i=0; i<width*height*3; ++i)
		data2[i] = (unsigned char) std::max(0, std::min(255, (int) std::floor(255.0 * canvas[i])));

	write_bmp(filename, data2, width, height);
}

int main(){
	read_bmp("reference-crop.bmp", &data, &width, &height);
	int diag = std::sqrt(width*width + height*height);
	data2 = new unsigned char[width*height*3];
	canvas = new double[width*height*3];
	
	for(int i=0; i<n; ++i){
		splat[i].x = rng()*width, splat[i].y = rng()*height;
		splat[i].r[0] = rng()*diag/16 + 3.0;
		splat[i].r[1] = rng()*diag/16 + 3.0;
		double theta = rng()*M_PI*2.0;
		splat[i].r[2] = std::cos(theta);
		splat[i].r[3] = -std::sin(theta);
		for(int j=0; j<4; ++j) splat[i].c[j] = rng();
	}

	read_splats("config.txt");

	paint();
	double error = loss();

	/*
	for(int i=0; i<n; ++i)
		for(int j=0; j<3; ++j)
			splat[i].c[j] = (double) data[
				((int)std::floor(splat[i].x)+
				 (int)std::floor(splat[i].y)*width)*3+j]/255.0;

	paint();
	if(loss() > error) read_splats("config.txt"); else error = loss();
	*/

	signal(SIGINT, sighandler);

	bool y = 1; while(true){
		if(signal_status == SIGINT){
			std::cout << "\nterminated";
			break;
		}

		auto start = std::chrono::high_resolution_clock::now();

		int i = rand()%n;
		_splat old = splat[i];

		splat[i].x = rng()*width, splat[i].y = rng()*height;
		splat[i].r[0] = rng()*diag/16;
		splat[i].r[1] = rng()*diag/16;
		double theta = rng()*M_PI*2.0;
		splat[i].r[2] = std::cos(theta);
		splat[i].r[3] = -std::sin(theta);
		for(int j=0; j<4; ++j) splat[i].c[j] = rng();

		paint();
		double new_error = loss();

		bool reject = 0;
		if(new_error > error) splat[i] = old, reject = 1;
		else error = new_error;

		auto now = std::chrono::high_resolution_clock::now();
		double duration = ((std::chrono::duration<double>) (now - start)).count();
		std::cout << '\r' << iter/1000 << "K  " << error << "E  " << (int) std::floor(duration*1000) << "T  ";
		std::flush(std::cout);

		if(iter%1000 == 0){
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

	write_splats("config.txt");
}

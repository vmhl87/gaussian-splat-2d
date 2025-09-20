#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

const int scale = 4;

#include "write_bmp.cpp"
#include "read_bmp.cpp"

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

void write(const char *filename){
	for(int i=0; i<width*height*3; ++i)
		data2[i] = (unsigned char) std::max(0, std::min(255, (int) std::floor(255.0 * canvas[i])));

	write_bmp(filename, data2, width, height);
}

int main(int argc, const char **argv){
	if(argc != 3){
		std::cerr << "Incorrect usage:\n";
		std::cerr << argv[0] << " <config.loc> <output.loc>\n";
		return 0;
	}

	read_bmp("reference-crop.bmp", &data, &width, &height);

	width *= scale, height *= scale;
	data2 = new unsigned char[width*height*3];
	canvas = new double[width*height*3];

	read_splats(argv[1]);

	for(int i=0; i<n; ++i){
		splat[i].x *= scale, splat[i].y *= scale;
		splat[i].r[0] *= scale, splat[i].r[1] *= scale;
	}

	auto start = std::chrono::high_resolution_clock::now();

	paint();

	auto now = std::chrono::high_resolution_clock::now();
	double duration = ((std::chrono::duration<double>) (now-start)).count();
	std::cout << "upscaled in " << (int) std::floor(duration*1000) << "ms\n";

	write(argv[2]);
}

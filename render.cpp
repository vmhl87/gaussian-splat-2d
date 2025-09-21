#include <sstream>
#include <fstream>
#include <thread>

void _paint(int xmin, int ymin, int xmax, int ymax){
	for(int i=0; i<n; ++i){

		auto SP = [] (float a, float b) { return std::sqrt(a*a+b*b); };
		float xr = SP(splat[i].r[0]*splat[i].r[2], splat[i].r[1]*splat[i].r[3]),
			  yr = SP(splat[i].r[0]*splat[i].r[3], splat[i].r[1]*splat[i].r[2]);

		xr *= 2.2;
		yr *= 2.2;

		int Xmin = std::max(xmin, (int) std::floor(splat[i].x-xr)),
			Xmax = std::min(xmax, (int) std::ceil(splat[i].x+xr)),
			Ymin = std::max(ymin, (int) std::floor(splat[i].y-yr)),
			Ymax = std::min(ymax, (int) std::ceil(splat[i].y+yr));

		for(int y=Ymin; y<Ymax; ++y){
			for(int x=Xmin; x<Xmax; ++x){
				int j = 3*(x+y*width);

				float rx = splat[i].x-x, ry = splat[i].y-y;
				float vx = (rx*splat[i].r[2]+ry*splat[i].r[3])/splat[i].r[0],
					  vy = (ry*splat[i].r[2]-rx*splat[i].r[3])/splat[i].r[1];
				float alpha = exp(-vx*vx-vy*vy) * splat[i].c[3];

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

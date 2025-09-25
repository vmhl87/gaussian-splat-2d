#include <sstream>
#include <fstream>
#include <thread>

void save_canvas(){
	std::memcpy(canvas2, canvas, sizeof(float)*width*height*3);
}

void restore_canvas(){
	std::memcpy(canvas, canvas2, sizeof(float)*width*height*3);
}

void bbox(_splat &s, int &xmin, int &ymin, int &xmax, int &ymax){
	auto SP = [] (float a, float b) { return std::sqrt(a*a+b*b); };
	float xr = SP(s.r[0]*s.r[2], s.r[1]*s.r[3]),
		  yr = SP(s.r[0]*s.r[3], s.r[1]*s.r[2]);

	xr *= 2.2;
	yr *= 2.2;

	xmin = std::max(0, (int) std::floor(s.x-xr));
	xmax = std::min(width, (int) std::ceil(s.x+xr) + 1);
	ymin = std::max(0, (int) std::floor(s.y-yr));
	ymax = std::min(height, (int) std::ceil(s.y+yr) + 1);
}

void _paint(int xmin, int ymin, int xmax, int ymax){
	for(int y=ymin; y<ymax; ++y){
		for(int x=xmin; x<xmax; ++x){
			for(int z=0; z<3; ++z){
				canvas[3*(x+y*width)+z] = 1.0;
			}
		}
	}

	for(int i=0; i<n; ++i){
		int Xmin, Ymin, Xmax, Ymax;
		bbox(splat[i], Xmin, Ymin, Xmax, Ymax);

		Xmin = std::max(xmin, Xmin);
		Ymin = std::max(ymin, Ymin);
		Xmax = std::min(xmax, Xmax);
		Ymax = std::min(ymax, Ymax);

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

void paint(int xmin, int ymin, int xmax, int ymax){
	std::thread threads[THREADS-1];

	for(int i=1; i<THREADS; ++i)
		threads[i-1] = std::thread(_paint,
			xmin + ((xmax-xmin)*(i-1))/THREADS, ymin,
			xmin + ((xmax-xmin)*i)/THREADS, ymax);

	_paint(xmin + ((xmax-xmin)*(THREADS-1))/THREADS, ymin, xmax, ymax);
	
	for(int i=1; i<THREADS; ++i) threads[i-1].join();
}

void paint(_splat &s){
	int xmin, ymin, xmax, ymax;
	bbox(s, xmin, ymin, xmax, ymax);
	paint(xmin, ymin, xmax, ymax);
}

void write(const char *filename){
	for(int i=0; i<width*height*3; ++i)
		data2[i] = (unsigned char) std::max(0, std::min(255, (int) std::floor(255.0 * canvas[i])));

	write_bmp(filename, data2, width, height);
}

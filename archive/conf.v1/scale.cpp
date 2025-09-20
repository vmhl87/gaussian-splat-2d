#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

#include "src/write_bmp.cpp"
#include "src/read_bmp.cpp"

#include "splat.cpp"

int main(int argc, const char **argv){
	if(argc != 4){
		std::cerr << "Incorrect usage:\n";
		std::cerr << argv[0] << " <config.loc> <output.loc> <scale>\n";
		return 0;
	}

	int scale = std::stoi(argv[3]);

	read_splats(argv[1]);

	width *= scale, height *= scale;
	data2 = new unsigned char[width*height*3];
	canvas = new double[width*height*3];

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

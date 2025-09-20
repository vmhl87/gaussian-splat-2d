#include "read_bmp.cpp"
#include "write_bmp.cpp"

int main(){
	unsigned char *data;
	int w, h;

	read_bmp("reference-full.bmp", &data, &w, &h);

	std::cout << w << 'x' << h << '\n';

	int left = 850,
		top = 1700,
		right = 2450,
		bottom = 2500,

		scale = 5,

		W = (right-left)/scale,
		H = (bottom-top)/scale;

	unsigned char *out = new unsigned char[W*H*3];

	for(int y=0; y<H; ++y)
		for(int x=0; x<W; ++x)
			for(int z=0; z<3; ++z){
				int sum = 0;
				for(int a=0; a<scale; ++a)
					for(int b=0; b<scale; ++b)
						sum += data[3*(x*scale+left+a+(y*scale+top+b)*w)+z];
				out[3*(x+y*W)+z] = (unsigned char) (sum/scale/scale);
			}

	write_bmp("reference-crop.bmp", out, W, H);
}

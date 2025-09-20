#include "read_bmp.cpp"
#include "write_bmp.cpp"

int main(){
	unsigned char *data;
	int w, h;

	read_bmp("reference.bmp", &data, &w, &h);

	unsigned char *out = new unsigned char[w*h*3];

	for(int y=0; y<h; ++y)
		for(int x=0; x<w; ++x)
			for(int z=0; z<3; ++z)
				out[3*(x+y*w)+z] = data[3*(x+y*w)+z];

	write_bmp("test.bmp", out, w, h);
}

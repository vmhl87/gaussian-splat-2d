#include <fstream>

void read_splats(const char *filename){
    std::ifstream F(filename);

    if(!F.is_open()) return;

	F >> iter;

	for(int i=0; i<n; ++i){
		F >> splat[i].x >> splat[i].y;
		for(int j=0; j<4; ++j) F >> splat[i].r[j];
		for(int j=0; j<4; ++j) F >> splat[i].c[j];
	}

    F.close();
}

void write_splats(const char *filename){
    std::ofstream F(filename);
	
	F << iter << '\n';

	for(int i=0; i<n; ++i){
		F << splat[i].x << '\n' << splat[i].y << '\n';
		for(int j=0; j<4; ++j) F << splat[i].r[j] << '\n';
		for(int j=0; j<4; ++j) F << splat[i].c[j] << '\n';
	}

    F.close();
}

const int VERSION = 5;

#include <fstream>
#include <cstring>

// defn

struct _splat{
	float x, y, r[5], c[4];
};

// signature

float loss(int xmin, int ymin, int xmax, int ymax);
void paint();

void init_state();

// glob

int n;
_splat *splat;

int iter = 0;

unsigned char *data2;
int width = -1, height = -1;
float diag, *canvas, *canvas2;

// imp

void init_state(){
	n = DEFAULT_SPLATS;

	splat = new _splat[n];

	for(int i=0; i<n; ++i){
		splat[i].x = rng()*width;
		splat[i].y = rng()*height;

		splat[i].r[0] = rng()*diag/16 + 3.0;
		splat[i].r[1] = rng()*diag/16 + 3.0;

		splat[i].r[4] = rng()*M_PI*2.0;
		splat[i].r[2] = std::cos(splat[i].r[4]);
		splat[i].r[3] = std::sin(splat[i].r[4]);

		for(int z=0; z<4; ++z) splat[i].c[z] = rng();
	}
}

#include "render.cpp"

int read_state(const char *filename){
    std::ifstream F(filename);

    if(!F.is_open()) return 1;

	std::string S; std::getline(F, S);

	if(S.length() < 8 || S.substr(0, 8) != "VERSION "){
		std::cerr << "CONFIG VERSION OUTDATED/MISSING\nABORTING\n";
		exit(1);

	}else{
		std::istringstream V(S);
		int v; std::string x; V >> x >> v;
		if(v != VERSION){
			std::cerr << "CONFIG VERSION OUTDATED: " << v << ':' << VERSION << "\nABORTING\n";
			exit(1);
		}
	}

	F >> n;

	if(n <= 0) std::cerr << "INVALID SPLAT COUNT: " << n << "\nABORTING\n", exit(1);

	splat = new _splat[n];

	F >> width >> height;

	F >> iter;

	char ch; F.get(ch);

	auto in = [&F] (float &f) {
		float ret;
		unsigned char *d = (unsigned char*) &ret;
		for(int i=0; i<sizeof(float); ++i){
			char ch; F.get(ch);
			d[i] = ch;
		}
		f = ret;
	};

	for(int i=0; i<n; ++i){
		in(splat[i].x), in(splat[i].y);
		in(splat[i].r[0]), in(splat[i].r[1]), in(splat[i].r[4]);
		splat[i].r[2] = std::cos(splat[i].r[4]);
		splat[i].r[3] = std::sin(splat[i].r[4]);
		for(int j=0; j<4; ++j) in(splat[i].c[j]);
	}

    F.close();

	return 0;
}

void write_state(const char *filename){
    std::ofstream F(filename);

	F << "VERSION " << VERSION << '\n';

	F << n << '\n';

	F << width << ' ' << height << '\n';
	
	F << iter << '\n';

	auto out = [&F] (float f){
		unsigned char *d = (unsigned char*) &f;
		for(int i=0; i<sizeof(float); ++i) F << d[i];
	};

	for(int i=0; i<n; ++i){
		out(splat[i].x), out(splat[i].y);
		out(splat[i].r[0]), out(splat[i].r[1]), out(splat[i].r[4]);
		for(int j=0; j<4; ++j) out(splat[i].c[j]);
	}

    F.close();
}

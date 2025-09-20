const int VERSION = 1;

#include <sstream>
#include <fstream>

struct _splat{
	double x, y, r[4], c[4];
};

int n;
_splat *splat;
int iter = 0;

unsigned char *data2;
int width, height;
double *canvas;

int read_splats(const char *filename){
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

	if(n > 0) splat = new _splat[n];
	else std::cerr << "INVALID SPLAT COUNT: " << n << "\nABORTING\n", exit(1);

	F >> width >> height;

	F >> iter;

	for(int i=0; i<n; ++i){
		F >> splat[i].x >> splat[i].y;
		for(int j=0; j<4; ++j) F >> splat[i].r[j];
		for(int j=0; j<4; ++j) F >> splat[i].c[j];
	}

    F.close();

	return 0;
}

void write_splats(const char *filename){
    std::ofstream F(filename);

	F << "VERSION " << VERSION << '\n';

	F << n << '\n';

	F << width << ' ' << height << '\n';
	
	F << iter << '\n';

	for(int i=0; i<n; ++i){
		F << splat[i].x << ' ' << splat[i].y << ' ';
		for(int j=0; j<4; ++j) F << splat[i].r[j] << ' ';
		for(int j=0; j<4; ++j) F << splat[i].c[j] << " \n"[j==3];
	}

    F.close();
}

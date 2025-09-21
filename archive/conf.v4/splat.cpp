const int VERSION = 4;

#include <fstream>
#include <cstring>

// defn

struct _splat{
	float x, y, r[5], c[4];
};

struct _state{
	_splat *data;
	float loss;
};

struct _particle{
	_state now, best;
};

// signature

float loss(_state &s);
void paint(_state &s);

void init_particles();
void copystate(_state &a, _state &b);

// glob

int n, particles;
_particle *particle;
_state best;

int iter = 0;

unsigned char *data2;
int width = -1, height = -1;
float diag, *canvas;

// imp

void copystate(_state &a, _state &b){
	std::memcpy(b.data, a.data, n * sizeof(_splat));
	b.loss = a.loss;
}

void init_state(_state &s){
	s.data = new _splat[n];
}

void init_particles(){
	n = DEFAULT_SPLATS;
	particles = DEFAULT_PARTICLES;

	init_state(best);
	particle = new _particle[particles];
	for(int i=0; i<particles; ++i) init_state(particle[i].now), init_state(particle[i].best);

	float best_loss = 1e9;
	int best_idx = -1;

	for(int i=0; i<particles; ++i){
		for(int j=0; j<n; ++j){
			particle[i].now.data[j].x = rng()*width;
			particle[i].now.data[j].y = rng()*height;

			particle[i].now.data[j].r[0] = rng()*diag/16 + 3.0;
			particle[i].now.data[j].r[1] = rng()*diag/16 + 3.0;

			particle[i].now.data[j].r[4] = rng()*M_PI*2.0;
			particle[i].now.data[j].r[2] = std::cos(particle[i].now.data[j].r[4]);
			particle[i].now.data[j].r[3] = std::sin(particle[i].now.data[j].r[4]);

			for(int z=0; z<4; ++z) particle[i].now.data[j].c[z] = rng();
		}

		if(loss(particle[i].now) < best_loss) best_loss = particle[i].now.loss, best_idx = i;

		copystate(particle[i].now, particle[i].best);
	}

	if(best_idx != -1) copystate(particle[best_idx].now, best);
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

	F >> particles >> n;

	if(particles <= 0) std::cerr << "INVALID PARTICLE COUNT: " << n << "\nABORTING\n", exit(1);
	if(n <= 0) std::cerr << "INVALID SPLAT COUNT: " << n << "\nABORTING\n", exit(1);

	init_state(best);
	particle = new _particle[particles];
	for(int i=0; i<particles; ++i) init_state(particle[i].now), init_state(particle[i].best);

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

	in(best.loss);
	for(int i=0; i<n; ++i){
		in(best.data[i].x), in(best.data[i].y);
		in(best.data[i].r[0]), in(best.data[i].r[1]), in(best.data[i].r[4]);
		best.data[i].r[2] = std::cos(best.data[i].r[4]);
		best.data[i].r[3] = std::sin(best.data[i].r[4]);
		for(int j=0; j<4; ++j) in(best.data[i].c[j]);
	}

	for(int x=0; x<particles; ++x){
		in(particle[x].best.loss);
		for(int i=0; i<n; ++i){
			in(particle[x].best.data[i].x), in(particle[x].best.data[i].y);
			in(particle[x].best.data[i].r[0]), in(particle[x].best.data[i].r[1]);
			in(particle[x].best.data[i].r[4]);
			particle[x].best.data[i].r[2] = std::cos(particle[x].best.data[i].r[4]);
			particle[x].best.data[i].r[3] = std::sin(particle[x].best.data[i].r[4]);
			for(int j=0; j<4; ++j) in(particle[x].best.data[i].c[j]);
		}
		in(particle[x].now.loss);
		for(int i=0; i<n; ++i){
			in(particle[x].now.data[i].x), in(particle[x].now.data[i].y);
			in(particle[x].now.data[i].r[0]), in(particle[x].now.data[i].r[1]);
			in(particle[x].now.data[i].r[4]);
			particle[x].now.data[i].r[2] = std::cos(particle[x].now.data[i].r[4]);
			particle[x].now.data[i].r[3] = std::sin(particle[x].now.data[i].r[4]);
			for(int j=0; j<4; ++j) in(particle[x].now.data[i].c[j]);
		}
	}

    F.close();

	return 0;
}

void write_state(const char *filename){
    std::ofstream F(filename);

	F << "VERSION " << VERSION << '\n';

	F << particles << ' ' << n << '\n';

	F << width << ' ' << height << '\n';
	
	F << iter << '\n';

	auto out = [&F] (float f){
		unsigned char *d = (unsigned char*) &f;
		for(int i=0; i<sizeof(float); ++i) F << d[i];
	};

	out(best.loss);
	for(int i=0; i<n; ++i){
		out(best.data[i].x), out(best.data[i].y);
		out(best.data[i].r[0]), out(best.data[i].r[1]);
		out(best.data[i].r[4]);
		for(int j=0; j<4; ++j) out(best.data[i].c[j]);
	}

	for(int x=0; x<particles; ++x){
		out(particle[x].best.loss);
		for(int i=0; i<n; ++i){
			out(particle[x].best.data[i].x), out(particle[x].best.data[i].y);
			out(particle[x].best.data[i].r[0]), out(particle[x].best.data[i].r[1]);
			out(particle[x].best.data[i].r[4]);
			for(int j=0; j<4; ++j) out(particle[x].best.data[i].c[j]);
		}
		out(particle[x].now.loss);
		for(int i=0; i<n; ++i){
			out(particle[x].now.data[i].x), out(particle[x].now.data[i].y);
			out(particle[x].now.data[i].r[0]), out(particle[x].now.data[i].r[1]);
			out(particle[x].now.data[i].r[4]);
			for(int j=0; j<4; ++j) out(particle[x].now.data[i].c[j]);
		}
	}

    F.close();
}

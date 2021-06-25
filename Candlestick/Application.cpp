#include <SDL.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include "GV.h"

#define WIDTH 1920
#define HEIGHT 1080

#define VERBOSE 1

struct Candlestick {
	double high, low, open, close;
	double x, candle_width;
	Candlestick(double h, double l, double o, double c, double x, double cw) : x(x), candle_width(cw), high(h), low(l), open(o), close(c) {}

	void Render(SDL_Renderer* r) const {
		if (open > close) {
			SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
		}
		else if(open < close) {
			SDL_SetRenderDrawColor(r, 0, 255, 0, 255);
		}


		SDL_RenderDrawLine(r, x + GV::panx, HEIGHT-high*GV::scale*3 + GV::pany, x + GV::panx, HEIGHT-low*GV::scale*3 + GV::pany);
		SDL_Rect candle = { x-candle_width/2 + GV::panx, HEIGHT-open*GV::scale*3 + GV::pany, candle_width, (open-close)*GV::scale*2 };
		SDL_RenderFillRect(r, &candle);
	}

};

struct Application {

private:
	SDL_Window* w;
	SDL_Renderer* r;

	std::string quote;

	bool RENDER_GRID = false;

	constexpr static uint32_t MAX_DATA = 65000;

	std::ifstream ifs;

	bool quit;

	std::string title;
	std::unordered_map<std::string, std::vector<std::array<double, 4>>> price_data;
	

public:

	void InitData(const std::string&& quote) {
		this->quote = quote;
		price_data.insert({ quote, {} });
	}

	void Log(const std::string& msg, uint32_t type) const noexcept {
		std::cerr << (type == 0 ? "[?]" : type == 1 ? "[-]" : type == 2 ? "[!]" : type == 3 ? "[FATAL]" : "[]") << " " << msg << std::endl;
	}

	std::unordered_map<std::string, std::vector<std::array<double, 4>>> GetPriceData() const noexcept {
		return price_data;
	}

	void Update() {
		std::string line;
		
		auto it = price_data.find(quote);
		uint32_t count = 0;
		if(it != price_data.end()) {
			while (std::getline(ifs, line)) {
				std::string date, high, low, open, close, adjclose, volume;
				std::stringstream ss(line);
				std::getline(ss, date, ',');
				std::getline(ss, open, ',');
				std::getline(ss, high, ',');
				std::getline(ss, low, ',');
				std::getline(ss, close, ',');
				std::getline(ss, adjclose, ',');
				std::getline(ss, volume, ',');

				#if VERBOSE 
					std::cout << high << " - " << low << " - " << close << " - " << adjclose << " - " << open << " - " << volume << "\n";
				#endif
				try {
					it->second.push_back({ std::stof(high), std::stof(low), std::stof(open), std::stof(close) });
				}
				catch (std::invalid_argument& e) { Log(e.what(), 2); }
			}
		}
		else {
			std::cout << quote << "\n";
			Log("Cannot find quote!" , 2);
		}
	}

	void Render() {
		for (auto& e : price_data) {
			double x = 0;
			for (auto it = e.second.cbegin(); it != e.second.cend(); it++) {

				Candlestick cs(it->at(0), it->at(1), it->at(2), it->at(3), x, GV::scale*2);
				cs.Render(r);
				x += WIDTH / e.second.size();
			}
		}
	}
	
	void EventLoop() {
		int zoom = 0;
		int drag = 0;
		SDL_Event e;
		while (!quit) {
			
			while (SDL_PollEvent(&e) != NULL) {
				switch (e.type) {
				case SDL_QUIT:
					quit = true;
					break;

				case SDL_KEYDOWN: {
					if (e.key.keysym.sym == SDLK_LCTRL) {
						zoom = 1;
						break;
					}
				}
				case SDL_KEYUP: {
					if (e.key.keysym.sym == SDLK_LCTRL) {
						zoom = 0;
						break;
					}
				}
				case SDL_MOUSEWHEEL: {
					if (e.wheel.y > 0) {
						if (zoom == 1) {
							GV::scale += 0.05;
							int x, y;
							SDL_GetMouseState(&x, &y);
							GV::panx += (WIDTH/2 - x) / 20;
							GV::pany += (HEIGHT / 2 - y) / 20;
						}
						else {
							GV::panx -= 50;
						}
					}
					else {
						if (zoom == 1) {
							GV::scale -= 0.05;
							int x, y;
							SDL_GetMouseState(&x, &y);
							GV::panx += (WIDTH / 2 - x) / 20;
							GV::pany += (HEIGHT / 2 - y) / 20;
						}
						else {
							GV::panx += 50;
						}
					}
				}
				case SDL_MOUSEBUTTONDOWN: {
					if (e.button.button == SDL_BUTTON_LEFT) {
						drag = 1;

						break;
					}
				}
				case SDL_MOUSEBUTTONUP: {
					if (e.button.button == SDL_BUTTON_LEFT) {
						drag = 0;

						break;
					}
				}
				case SDL_MOUSEMOTION: {

					if (drag == 1) {
						GV::panx += 0.4 * e.motion.xrel;
						GV::pany += 0.4 * e.motion.yrel;
					}
				
				}
				}
			}
			SDL_SetRenderDrawColor(r, 20, 20, 20, 255);
			SDL_RenderClear(r);
			
			if (RENDER_GRID) {
				// render grid
				SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
				SDL_RenderSetScale(r, 1, 1);
				for (uint32_t i = 0; i < WIDTH; i += GV::scale * 10) {
					SDL_RenderDrawLine(r, i, 0, i, HEIGHT);
				}
				for (uint32_t i = 0; i < HEIGHT; i += GV::scale * 10) {
					SDL_RenderDrawLine(r, 0, i, WIDTH, i);
				}
			}
			SDL_RenderSetScale(r, GV::scale, GV::scale);
			Render();

			SDL_RenderPresent(r);
		}

		SDL_DestroyRenderer(r);
		SDL_DestroyWindow(w);

		SDL_Quit();
	}

	Application(const std::string&& title, std::string&& quote) : title(title), quit(false) {
		Log("Application starting", 0);

		GV::scale = 1;

		InitData(std::forward<std::string>(quote));
		SDL_Init(SDL_INIT_VIDEO);
		w = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_RENDERER_ACCELERATED);
		r = SDL_CreateRenderer(w, -1, NULL);

		std::string filename(quote);
		filename.append(".csv");
		ifs = std::ifstream(filename.c_str());
		if (!ifs.good()) {
			Log("cannot open data file!", 2);
			return;
		}

		ifs.ignore(256, '\n');

		Update();

		EventLoop();
	}

	~Application() {
		Log("Application exiting", 0);
		price_data = std::unordered_map<std::string, std::vector<std::array<double, 4>>>();

		r = nullptr;
		w = nullptr;

		ifs.close();

	}

};

int main(int argc, char** argv) {
	std::unique_ptr<Application> app = std::make_unique<Application>("App - AAPL", "AAPL");
	return 0;
}
#include <raylib.h>
#include <random>

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 600;
static constexpr int FONT_SIZE = 20;
static constexpr int GAP_SIZE = SCREEN_HEIGHT / 3;
static constexpr Color TEXT_COLOR = MAROON;
static constexpr int PLAYER_RADIUS = 15;

static Font FONT;

enum class GameMode {
	Menu,
	Playing,
	End,
	Quitting,
};

class State;
class Obstacle;

class Player final {
private:
	static constexpr float DRAGON_MASS = 1.0;
	static constexpr float HORIZONTAL_VELOCITY = 120.0;
	static constexpr float GRAV_ACCELERATION = 600.0;
	static constexpr float FLAP_FORCE = -20000.0;

	Vector2 pos{0, 0};
	Vector2 vel{HORIZONTAL_VELOCITY, 0};
	Vector2 acc{0, GRAV_ACCELERATION};
	Vector2 force_accum{0, 0};
	float inverse_mass = 1.0 / DRAGON_MASS;

	void add_force(float fx, float fy) {
		force_accum.x += fx;
		force_accum.y += fy;
	}
public:
	Player(int x, int y) {
		pos.x = x;
		pos.y = y;
	}

	void render() {
		DrawCircle(PLAYER_RADIUS, pos.y, PLAYER_RADIUS, RED);
	}

	void physics(float dt) {
		pos.x += vel.x * dt; 	
		pos.y += vel.y * dt;

		Vector2 accel = acc;
		accel.x += force_accum.x * inverse_mass;
		accel.y += force_accum.y * inverse_mass;

		vel.x += accel.x * dt;
		vel.y += accel.y * dt;
		if (pos.y < 0.0) {
			pos.y = 0.0;
			vel.y = 0.0;
		}

		force_accum.x = force_accum.y = 0.0;
	}

	void flap() {
		vel.y = 0.0;
		add_force(0.0, FLAP_FORCE);	
	}

	friend class State;
	friend class Obstacle;
};

class Obstacle final {
private:
	static constexpr int OBSTACLE_WIDTH = SCREEN_WIDTH / 20;
	static constexpr int GROUND_HEIGHT = 15;
	int x;
	int gap;
	int size;
public:
	Obstacle(int x, int gap, int size) : x(x), gap(gap), size(size) {}

	static Obstacle create(int x, int score) {
		static std::random_device r;
		std::default_random_engine e(r());
		std::uniform_int_distribution<int> u(SCREEN_HEIGHT/9, (SCREEN_HEIGHT * 8) / 10);
		return Obstacle{x, u(e), GAP_SIZE};
	}

	void render(int player_x) {
		const int screen_x = x - player_x;
		const int half_size = size / 2;
		
		// top
		DrawRectangle(screen_x, 0, OBSTACLE_WIDTH, gap - half_size, BLUE);

		// bottom
		DrawRectangle(screen_x, gap+half_size, OBSTACLE_WIDTH, SCREEN_HEIGHT - gap - half_size, BLUE);
		DrawRectangle(0, SCREEN_HEIGHT - GROUND_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, DARKGREEN);
	}

	bool is_hit(const Player &player) const {
		const float half_size = size / 2.0;
		Rectangle upper = { 1.0f * x, 0.0, OBSTACLE_WIDTH, gap - half_size };
		Rectangle lower = { 1.0f * x, gap + half_size, OBSTACLE_WIDTH, SCREEN_HEIGHT - gap - half_size};
		return CheckCollisionCircleRec(player.pos, PLAYER_RADIUS, upper) ||
		       CheckCollisionCircleRec(player.pos, PLAYER_RADIUS, lower);
	}

	friend class State;
};

class State final {
private:
	GameMode mode_ = GameMode::Menu;
	Player player_{5, SCREEN_HEIGHT / 2};
	Obstacle obstacle_ = Obstacle::create(SCREEN_WIDTH, 0);
	int score_ = 0;
	bool can_flap_ = true;

	static constexpr const char *WELCOME_TEXT = "Welcome to Flappy Dragon";
	static constexpr const char *FLAP_TEXT = "Press SPACE to flap";
	static constexpr const char *PLAY_GAME = "(P) Play Game";
	static constexpr const char *PLAY_AGAIN = "(P) Play Again";
	static constexpr const char *QUIT_GAME = "(Q) Quit Game";
	static constexpr const char *YOU_ARE_DEAD_TEXT = "You're Dead!";

	static auto welcome_text_len() {
		static auto len = MeasureTextEx(FONT, WELCOME_TEXT, FONT.baseSize, 2);
		return len;
	}

	static auto flap_text_len() {
		static auto len = MeasureTextEx(FONT, FLAP_TEXT, FONT.baseSize, 2);
		return len;
	}

	static auto dead_text_len() {
		static auto len = MeasureTextEx(FONT, YOU_ARE_DEAD_TEXT, FONT.baseSize, 2);
		return len;
	}
public:
	State() = default;
	State(const State&) = delete;
	State& operator=(const State&) = delete;

	GameMode mode() const { return mode_; }

	void on_main_menu() {
		BeginDrawing();
		ClearBackground(WHITE);

		auto wtl = welcome_text_len();
		float xloc = (SCREEN_WIDTH - wtl.x) / 2.0;
		Vector2 fpos = { xloc, SCREEN_HEIGHT / 3.0 };
		DrawTextEx(FONT, WELCOME_TEXT, fpos, FONT.baseSize, 2, TEXT_COLOR);

		fpos.y += wtl.y;
		DrawTextEx(FONT, PLAY_GAME   , fpos, FONT.baseSize, 2, TEXT_COLOR);

		fpos.y += wtl.y;
		DrawTextEx(FONT, QUIT_GAME   , fpos, FONT.baseSize, 2, TEXT_COLOR);
		EndDrawing();

		if (IsKeyDown(KEY_P)) {
			restart();
		} else if (IsKeyDown(KEY_Q)) {
			mode_ = GameMode::Quitting;
		}	
	}

	void on_play() {
		BeginDrawing();
		ClearBackground(WHITE);

		auto ftl = flap_text_len();
		Vector2 fpos = { 10.0, 10.0 };
		DrawTextEx(FONT, FLAP_TEXT, fpos, FONT.baseSize, 2, TEXT_COLOR);

		static char score_buffer[128];
		sprintf(score_buffer, "Score: %d", score_);

		fpos.y += ftl.y;
		DrawTextEx(FONT, score_buffer, fpos, FONT.baseSize, 2, TEXT_COLOR);

		auto frame_time = GetFrameTime();
		player_.physics(frame_time);
		if (can_flap_ && IsKeyDown(KEY_SPACE)) {
			player_.flap();
			can_flap_ = false;
		}
		if (!can_flap_ && IsKeyUp(KEY_SPACE)) {
			can_flap_ = true;
		}
		player_.render();
		obstacle_.render(player_.pos.x);
		EndDrawing();

		if (player_.pos.y > SCREEN_HEIGHT || obstacle_.is_hit(player_)) {
			mode_ = GameMode::End;
		} else if (player_.pos.x > obstacle_.x) {
			score_ += 1;
			obstacle_ = Obstacle::create(player_.pos.x + SCREEN_WIDTH, score_);
		}
	}

	void on_died() {
		BeginDrawing();
		ClearBackground(WHITE);

		auto dtl = dead_text_len();
		Vector2 loc = { (SCREEN_WIDTH - dtl.x) / 2, SCREEN_HEIGHT / 3.0};
		DrawTextEx(FONT, YOU_ARE_DEAD_TEXT, loc, FONT.baseSize, 2, TEXT_COLOR);

		loc.y += dtl.y;
		DrawTextEx(FONT, PLAY_AGAIN, loc, FONT.baseSize, 2, TEXT_COLOR);

		loc.y += dtl.y;
		DrawTextEx(FONT, QUIT_GAME, loc, FONT.baseSize, 2, TEXT_COLOR);
		EndDrawing();

		if (IsKeyDown(KEY_P)) {
			restart();
		} else if (IsKeyDown(KEY_Q)) {
			mode_ = GameMode::Quitting;
		}
	}

	void restart() {
		mode_ = GameMode::Playing;
		player_ = Player(5, SCREEN_HEIGHT / 2.0);
		obstacle_ = Obstacle::create(SCREEN_WIDTH, 0);
		score_ = 0;
	}
};//~ State

int main() {
	SetTargetFPS(60);
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy Dragon");

	FONT = LoadFont("../resources/pixantiqua.fnt");

	State state;
	bool quit = false;
	while (!WindowShouldClose() && !quit) {
		switch (state.mode()) {
		case GameMode::Menu:
			state.on_main_menu();
			break;
		case GameMode::Playing:
			state.on_play();
			break;
		case GameMode::End:
			state.on_died();
			break;
		case GameMode::Quitting:
			quit = true;
		default:
			break;
		}
	}
	UnloadFont(FONT);
	CloseWindow();
	return 0;
}

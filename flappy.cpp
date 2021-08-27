#include <raylib.h>
#include <random>

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 600;
static constexpr int FONT_SIZE = 20;
static constexpr int GAP_SIZE = SCREEN_HEIGHT / 3;

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
		DrawText("@", 0, pos.y, 20, BLACK);
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
		for (int i = 0; i <= gap - half_size; ++i) {
			DrawText("#", screen_x, i, FONT_SIZE, BLUE);
		}

		// bottom
		for (int i = gap + half_size; i < SCREEN_HEIGHT; ++i) {
			DrawText("#", screen_x, i, FONT_SIZE, BLUE);
		}

		for (int i = 0; i < SCREEN_WIDTH; ++i) 
			DrawText("#", i, SCREEN_HEIGHT - 15, FONT_SIZE, DARKGREEN);
	}

	bool is_hit(const Player &player) const {
		const int half_size = size / 2;
		const bool x_matches = static_cast<int>(player.pos.x) == x;
		const bool above = static_cast<int>(player.pos.y) < gap - half_size;
		const bool below = static_cast<int>(player.pos.y) > gap + half_size;
		return x_matches && (above || below);
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
	static constexpr const char *PLAY_GAME = "(P) Play Game";
	static constexpr const char *PLAY_AGAIN = "(P) Play Again";
	static constexpr const char *QUIT_GAME = "(Q) Quit Game";
	static constexpr const char *YOU_ARE_DEAD_TEXT = "You're Dead!";

	static int welcome_text_len() {
		static int len = MeasureText(WELCOME_TEXT, FONT_SIZE);
		return len;
	}

	static int dead_text_len() {
		static int len = MeasureText(YOU_ARE_DEAD_TEXT, FONT_SIZE);
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

		int loc = (SCREEN_WIDTH - welcome_text_len()) / 2;
		DrawText(WELCOME_TEXT, loc, SCREEN_HEIGHT / 3, FONT_SIZE, BLACK);
		DrawText(PLAY_GAME, loc, SCREEN_HEIGHT / 3 + FONT_SIZE, FONT_SIZE, BLACK);
		DrawText(QUIT_GAME, loc, SCREEN_HEIGHT / 3 + FONT_SIZE * 2, FONT_SIZE, BLACK);
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
		DrawText("Press SPACE to flap", 10, 10, FONT_SIZE, BLACK);

		static char score_buffer[128];
		sprintf(score_buffer, "Score: %d", score_);
		DrawText(score_buffer, 10, 30, FONT_SIZE, BLACK);

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
		int loc = (SCREEN_WIDTH - dead_text_len()) / 2;
		DrawText(YOU_ARE_DEAD_TEXT, loc, SCREEN_HEIGHT / 3, FONT_SIZE, BLACK);
		DrawText(PLAY_AGAIN, loc, SCREEN_HEIGHT / 3 + FONT_SIZE, FONT_SIZE, BLACK);
		DrawText(QUIT_GAME, loc, SCREEN_HEIGHT / 3 + FONT_SIZE * 2, FONT_SIZE, BLACK);
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
	CloseWindow();
	return 0;
}

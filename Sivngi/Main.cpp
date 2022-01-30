# include <Siv3D.hpp> // OpenSiv3D v0.6.3

# include "QuadTree.hpp"

constexpr Rect gamearea{ 160, 160, 960, 640 };

namespace user {
	using ObjectShape = Circle;

	//衝突判定する物体
	struct Object : public ObjectShape
	{
		Vec2 speed = RandomVec2();

		//何かに衝突したか(Updateでリセット)
		bool collision = false;

		[[nodiscard]] Object()
			: ObjectShape(Arg::center = RandomVec2(gamearea), Random(5, 10))
		{
		}

		void Update()
		{
			collision = false;

			if (not gamearea.intersects(moveBy(speed))) // out of gamearea
				setCenter(RandomVec2(gamearea)); // reset pos
		}
	};
}

namespace sample {
	Rect drawQuadTreeGrid(const QuadTreeConfig& qtc, const ColorF& color = Palette::White)
	{
		const auto sec = 1 << qtc.lowestLayer;
		const auto sectionSize = qtc.gamearea.size.movedBy(sec - 1, sec - 1) / sec;

		auto [w, h] = sectionSize * sec;

		for (int i = 1; i < sec; ++i)
		{
			auto [x, y] = sectionSize * i;
			Line(x + 0.5, 1, x + 0.5, h - 1).movedBy(qtc.gamearea.pos).draw(1.0, color);
			Line(1, y + 0.5, w - 1, y + 0.5).movedBy(qtc.gamearea.pos).draw(1.0, color);
		}
		return qtc.gamearea.drawFrame(1.0, 0.0, color);
	}
}

void Main()
{
	Window::Resize(1280, 960);

	//四分木
	auto quadtree = QuadTree<user::Object>{ QuadTreeConfig(6, gamearea) };
	//objects
	Array<user::Object> objects(512);

	int intersectCheckCount = 0;
	int prevClock = 0;
	bool isNaive = true;

	while (System::Update())
	{
		ClearPrint();
		Print << U"N: Naiveな処理(QuadTreeを使わない)\nL: 衝突判定したobject組を表示\nSpace: objectの動きを止める";

		if (not KeySpace.pressed()) // not paused
		{
			//update objects
			objects.each([](auto& e) { e.Update(); });

			intersectCheckCount = 0;
			const auto whenIntersects = [&, vis = KeyL.pressed()](user::Object& a, user::Object& b)
			{
				// for sample
				if (vis)
					Line(a.center, b.center).draw(AlphaF(0.2));
				++intersectCheckCount;

				// 衝突判定を使うようななんかの処理(適当)
				// ユーザーが実際に書く部分
				if (a.intersects(b))
					a.collision = b.collision = true;
			};

			MicrosecClock clock{}; //処理時間計測 ----------------

			if (isNaive = KeyN.pressed())
			{
				// naive
				for (size_t i = 0, e = objects.size(); i < e; ++i)
					for (size_t k = i + 1; k < e; ++k)
						whenIntersects(objects[i], objects[k]);
			}
			else
			{
				quadtree(objects)(whenIntersects);
			}

			prevClock = clock.us(); //---------------- 処理時間計測

		}

		Print << (isNaive ? U"Naiveな処理" : U"QuadTreeで処理");
		Print << prevClock << U"us";

		//Draw
		{
			//log
			const auto n = objects.size();
			Print << U"n={}\tcombi(nC2)={}\tchecked:{}"_fmt(n, n * (n - 1) / 2, intersectCheckCount);

			//objects
			for (const auto& e : objects)
				e.draw(e.collision ? Palette::Red : Palette::Lightgreen);

			//QuadTree Grid
			sample::drawQuadTreeGrid(quadtree.currentConfig(), AlphaF(0.3));
		}
	}
}

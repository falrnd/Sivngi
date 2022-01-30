# Sivngi
(Region)QuadTree for [OpenSiv3D](https://github.com/Siv3D/OpenSiv3D)  

[OpenSiv3D](https://github.com/Siv3D/OpenSiv3D)向けの領域四分木の実装です。OpenSiv3D本体への採用を目標としています。

# 参考
http://marupeke296.com/COL_2D_No8_QuadTree.html  
https://qiita.com/Yujiro-Ito/items/1078db2d78f92898b813  


# サンプル(Main.cpp)
<td align="center"><img alt="sample" src="https://user-images.githubusercontent.com/28914324/151689363-85a67bb9-df49-4469-bde3-469b2b4c1582.png" ></td>  

```cpp
#include <Siv3D.hpp> // OpenSiv3D v0.6.3

#include "QuadTree.hpp"

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

			if (not KeySpace.pressed()) // not paused
				if (not gamearea.intersects(moveBy(speed))) // out of gamearea
					setCenter(RandomVec2(gamearea)); // reset pos
		}
	};
}

namespace sample {
	struct Profiler
	{
		static constexpr int loglen = 60;
		Array<double> exectimelog = Array<double>(loglen, 0.0);

		double UpdateAveExecTime(Stopwatch stopwatch)
		{
			exectimelog[Scene::FrameCount() % loglen] = stopwatch.msF();
			return exectimelog.sumF() / loglen;
		}
	};

	Rect drawDebugGrid(const QuadTreeConfig& qtc, const ColorF& color = Palette::White)
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

	//負荷表示用
	sample::Profiler profiler{};

	while (System::Update())
	{
		ClearPrint();
		Print << U"N: Naiveな処理(QuadTreeを使わない)\nL: 衝突判定したobject組を表示\nSpace: objectの動きを止める";

		//update objects
		objects.each([](auto& e) { e.Update(); });

		int intersectCheckCount = 0;
		const auto whenIntersects = [&, vis = KeyL.pressed()](user::Object& a, user::Object& b) {
			// for sample
			if (vis)
				Line(a.center, b.center).draw(AlphaF(0.2));
			++intersectCheckCount;

			// 衝突判定を使うようななんかの処理(適当)
			// ユーザーが実際に書く部分
			if (a.intersects(b))
			{
				a.collision = b.collision = true;
			}
		};

		Stopwatch stopwatch{ StartImmediately::Yes }; //処理時間計測 --------------------------

		if (KeyN.pressed())
		{
			Print << U"Naiveな処理";
			// naive
			for (size_t i = 0, e = objects.size(); i < e; ++i)
				for (size_t k = i + 1; k < e; ++k)
					whenIntersects(objects[i], objects[k]);
		}
		else
		{
			Print << U"QuadTreeで処理";
			quadtree(objects)(whenIntersects);
		}

		stopwatch.pause(); //----------------------------------------------------- 処理時間計測 


		//Draw
		{
			using namespace sample;

			//log
			const auto n = objects.size();
			Print << U"n={}\tcombi(nC2)={}\tchecked:{}"_fmt(n, n * (n - 1) / 2, intersectCheckCount);
			Print << profiler.UpdateAveExecTime(stopwatch) << U"ms";

			//objects
			for (const auto& e : objects)
				e.draw(e.collision ? Palette::Red : Palette::Lightgreen);

			//QuadTree Grid
			drawDebugGrid(quadtree.currentConfig(), AlphaF(0.3));
		}
	}
}
```

# サンプル(速度計測抜き)
```cpp
#include <Siv3D.hpp> // OpenSiv3D v0.6.3

#include "QuadTree.hpp"

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

			if (not KeySpace.pressed()) // not paused
				if (not gamearea.intersects(moveBy(speed))) // out of gamearea
					setCenter(RandomVec2(gamearea)); // reset pos
		}
	};
}

void Main()
{
	Window::Resize(1280, 960);

	//四分木
	auto quadtree = QuadTree<user::Object>{ QuadTreeConfig(6, gamearea) };
	//objects
	Array<user::Object> objects(512);

	while (System::Update())
	{
		ClearPrint();
		Print << U"L: 衝突判定したobject組を表示\nSpace: objectの動きを止める";

		//update objects
		objects.each([](auto& e) { e.Update(); });

		int intersectCheckCount = 0;
		const auto whenIntersects = [&, vis = KeyL.pressed()](user::Object& a, user::Object& b) {
			// for sample
			if (vis)
				Line(a.center, b.center).draw(AlphaF(0.2));
			++intersectCheckCount;

			// 衝突判定を使うようななんかの処理(適当)
			// ユーザーが実際に書く部分
			if (a.intersects(b))
			{
				a.collision = b.collision = true;
			}
		};

		quadtree(objects)(whenIntersects);


		//Draw
		{
			//log
			const auto n = objects.size();
			Print << U"n={}\tcombi(nC2)={}\tchecked:{}"_fmt(n, n * (n - 1) / 2, intersectCheckCount);

			//objects
			for (const auto& e : objects)
				e.draw(e.collision ? Palette::Red : Palette::Lightgreen);

			gamearea.drawFrame(1.0, 0.0);
		}
	}
}
```

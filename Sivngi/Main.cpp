#include <Siv3D.hpp> // OpenSiv3D v0.4.2

#include "QuadTree.hpp"

constexpr Rect window{ 1280, 960 };
constexpr Rect region{ 160, 160, 960, 640 };

namespace user {
	//using ObjectShape = RectF;
	using ObjectShape = Circle;

	//衝突判定する物体
	struct Object : public ObjectShape
	{
		Vec2 speed = RandomVec2();

		//何かに衝突したか(Updateでリセット)
		bool collision = false;

		[[nodiscard]] Object()
			: ObjectShape(Arg::center = RandomVec2(region), Random(5, 10))
		{
		}

		void Update()
		{
			collision = false;

			if (not KeySpace.pressed())
				if (not region.intersects(moveBy(speed)))
					setCenter(RandomVec2(region));
		}
	};

	auto center(const RectF& s) { return s.center(); }
	const auto& center(const Circle& s) { return s.center; }
}

namespace sample {
	template <class T>
	auto drawHistogram(const Array<T>& data, const RectF& bound, const ColorF& color = Palette::White)
	{
		T max = *std::max_element(data.cbegin(), data.cend());

		if (max != 0)
		{
			double width = bound.w / data.size();

			for (auto [i, v] : Indexed(data))
			{
				RectF(Arg::bottomLeft = bound.bl().movedBy(width * i, 0),
					width, bound.h * v / max)
					.draw(color);
			}
		}

		return bound;
	}

	Rect drawDebugGrid(const QuadTreeConfig& qtc, const ColorF& color = Palette::White)
	{
		const auto sec = 1 << qtc.lowestLayer;
		const auto sectionSize = qtc.region.size.movedBy(sec - 1, sec - 1) / sec;

		auto [w, h] = sectionSize * sec;

		for (int i = 1; i < sec; ++i)
		{
			auto [x, y] = sectionSize * i;
			Line(x + 0.5, 1, x + 0.5, h - 1).movedBy(qtc.region.pos).draw(1.0, color);
			Line(1, y + 0.5, w - 1, y + 0.5).movedBy(qtc.region.pos).draw(1.0, color);
		}
		return qtc.region.drawFrame(1.0, 0.0, color);
	}
}


void Main()
{
	// Space : ポーズ
	// L : 衝突関係を表示
	// Q : 衝突の確認にナイーブな実装を使用 (Qを押していない間は四分木を使用)

	Window::Resize(window.size);

	//負荷表示用
	Array<double> chartdata(window.w / 2);

	//四分木
	auto quadtree = QuadTree<user::Object>{ QuadTreeConfig(6, region) };

	//物体
	Array<user::Object> objects(512);

	while (System::Update())
	{
		ClearPrint();

		//update objects
		objects.each([](auto& e) { e.Update(); });

		//clear histogram
		if (KeyG.down())
		{
			for (auto& o : chartdata)
				o = 0;
		}

		int count = 0;

		// 衝突判定を使うようななんかの処理(適当)
		// ユーザーが実際に書く部分
		const auto objectRoutine = [&, vis = KeyL.pressed()](user::Object& a, user::Object& b) {
			if (vis)
			{
				Line(center(a), center(b))
					.draw(AlphaF(0.2));
			}
			if (a.intersects(b))
			{
				a.collision = b.collision = true;
			}
			++count;
		};

		//処理時間計測 -----------------------------------------------------
		Stopwatch stopwatch{ StartImmediately::Yes };

		if (KeyN.pressed())
		{
			Print << U"Naive";
			// naive
			for (size_t i = 0, e = objects.size(); i < e; ++i)
			{
				auto& a = objects[i];
				for (size_t j = i + 1; j < e; ++j)
				{
					auto& b = objects[j];
					// 衝突判定を使うようななんかの処理(適当)
					// ユーザーが実際に書く部分
					objectRoutine(a, b);
				}
			}
		}
		/*
		else if (KeyI.pressed())
		{
			Print << U"QuadTreeIterator";
			for (auto [a, b] : quadtree.Apply(objects))
			{
				// 衝突判定を使うようななんかの処理(適当)
				// ユーザーが実際に書く部分
				objectRoutine(a, b);
			}
		}
		*/
		else
		{
			Print << U"QuadTree";
			quadtree(objects)(objectRoutine);
		}

		stopwatch.pause();
		//----------------------------------------------------- 処理時間計測 

		//Draw
		{
			using namespace sample;
			//chart
			drawHistogram(chartdata, Rect(0, 720, window.w, 240), Palette::Skyblue);

			//log
			{
				const auto n = objects.size();
				Print << U"n={}\tcombi(nC2)={}\tchecked:{}"_fmt(n, n * (n - 1) / 2, count);

				const auto framecount = Scene::FrameCount();
				chartdata[framecount % chartdata.size()] = stopwatch.msF();

				double ave = 0;
				for (size_t i = 0; i < 60; ++i)
					ave += chartdata[(framecount - i + chartdata.size()) % chartdata.size()];
				Print << ave / 100 << U"ms";
			}

			//objects
			for (const auto& e : objects)
			{
				e.draw(e.collision ? Palette::Red : Palette::Lightgreen);
			}

			//QuadTree Grid
			drawDebugGrid(quadtree.currentConfig(), AlphaF(0.3));
		}
	}
}

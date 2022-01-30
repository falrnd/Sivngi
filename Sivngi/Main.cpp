# include <Siv3D.hpp> // OpenSiv3D v0.6.3

# include "QuadTree.hpp"

constexpr Rect GameArea{ 160, 160, 960, 640 };

using ObjectShape = Circle;

// 衝突判定する物体
struct MyObject : public ObjectShape
{
	Vec2 speed = RandomVec2();

	// 何かに衝突したか ( update() でリセット)
	bool collision = false;

	MyObject()
		: ObjectShape{ Arg::center = RandomVec2(GameArea), Random(5, 10) } {}

	void update()
	{
		collision = false;

		if (not GameArea.intersects(moveBy(speed))) // out of gamearea
		{
			setCenter(RandomVec2(GameArea)); // reset pos
		}
	}
};

// グリッドの描画
Rect DrawQuadTreeGrid(const QuadTreeConfig& qtc, const ColorF& color = Palette::White)
{
	const int32 sec = (1 << qtc.lowestLayer);
	const Size sectionSize = (qtc.gamearea.size.movedBy(sec - 1, sec - 1) / sec);
	const auto [w, h] = (sectionSize * sec);

	for (int32 i = 1; i < sec; ++i)
	{
		auto [x, y] = sectionSize * i;
		Line{ x + 0.5, 1, x + 0.5, h - 1 }.movedBy(qtc.gamearea.pos).draw(1.0, color);
		Line{ 1, y + 0.5, w - 1, y + 0.5 }.movedBy(qtc.gamearea.pos).draw(1.0, color);
	}

	return qtc.gamearea.drawFrame(1.0, 0.0, color);
}

void Main()
{
	Window::Resize(1280, 960);

	// 四分木
	QuadTree<MyObject> quadtree{ QuadTreeConfig{ 6, GameArea } };

	// 衝突判定する物体
	constexpr int32 N = 512;
	Array<MyObject> objects(N);

	// 設定・プロファイリング用の変数
	bool useQuadTree = true;
	bool showCombination = false;
	bool animation = true;
	int32 checkCount = 0;
	Array<int64> usBuffer(30);

	while (System::Update())
	{
		if (animation)
		{
			objects.each([](auto& e) { e.update(); });

			checkCount = 0;

			// ユーザ実装関数
			const auto onCollisionCheck = [&checkCount, showCombination](MyObject& a, MyObject& b)
			{
				if (showCombination)
				{
					Line{ a.center, b.center }.draw(AlphaF(0.2));
				}

				++checkCount;

				if (a.intersects(b))
				{
					a.collision = b.collision = true;
				}
			};

			{
				MicrosecClock clock;

				if (useQuadTree) // 四分木を使用
				{
					quadtree(objects)(onCollisionCheck);
				}
				else // 全探索
				{
					for (size_t i = 0; i < objects.size(); ++i)
					{
						for (size_t k = i + 1; k < objects.size(); ++k)
						{
							onCollisionCheck(objects[i], objects[k]);
						}
					}
				}

				const auto us = clock.us();
				usBuffer.rotate(1);
				usBuffer.back() = us;
			}
		}

		// 描画
		{
			for (const auto& object : objects)
			{
				object.draw(object.collision ? Palette::Red : Palette::Lightgreen);
			}

			// QuadTree Grid
			DrawQuadTreeGrid(quadtree.currentConfig(), AlphaF(0.3));
		}

		// デバッグ表示
		{
			const size_t n = objects.size();
			const size_t c = (n * (n - 1) / 2);

			ClearPrint();
			Print << U"N: {}"_fmt(n);
			Print << U"処理した組み合わせ: {} / {} ({:.1f}%)"_fmt(checkCount, c, (100.0 * checkCount / c));
			Print << U"{:.0f} us"_fmt(Statistics::Mean(usBuffer.begin(), usBuffer.end()));
		}

		// 設定
		{
			SimpleGUI::CheckBox(useQuadTree, U"QuadTree", Vec2{ 500, 20 });
			SimpleGUI::CheckBox(showCombination, U"Show Combinations", Vec2{ 660, 20 });
			SimpleGUI::CheckBox(animation, U"Animation", Vec2{ 920, 20 });
		}
	}
}

# Sivngi
(Region)QuadTree for [OpenSiv3D](https://github.com/Siv3D/OpenSiv3D)  

[OpenSiv3D](https://github.com/Siv3D/OpenSiv3D)向けの領域四分木の実装です。OpenSiv3D本体への採用を目標としています。

# 参考
http://marupeke296.com/COL_2D_No8_QuadTree.html  
https://qiita.com/Yujiro-Ito/items/1078db2d78f92898b813  

# Done
* オブジェクトの登録(**ただし現状Circle決め打ち(サンプルに合わせて)**)
* 四分木を走査し衝突可能性のあるオブジェクト組を列挙
  * `quadtree(object)(衝突しそうな組を受け取る関数オブジェクト)`

# ToDo(というかやりたい)
* Circle以外の型のObjectへの対応
  * boundingRectをどうもらうか(`std::function<Rect()>`を貰う or ユーザーに関数を作ってもらう(`std::begin`みたいな))

* オブジェクトの四分木への登録
  * 今の実装 : 四分木の内部コンテナを毎回clearして全オブジェクト登録しなおす(これでも全obj間衝突判定よりは速い)
  * 参考サイトの主張 : 移動した結果登録空間が変化したオブジェクトのみを移動(コンテナがlistなので`O(1)`)
    * 四分木に更新をリクエストする関数を作る?
    * コンテナは今は全部`Array`にしてるが`std::list`とどっちが速いか
    * あるいは`std::deque`

* 四分木を走査し衝突可能性のあるオブジェクト組を列挙
  * イテレータで渡したい `Pair<Obj,Obj>`みたいな
    * 3,4重ループの状態を保持するの面倒(良い書き方はあるのか？)
    * coroutineの出番?
  * 異なる型を格納した2つのQuadTree間で衝突判定
  * 異なる管理領域を持つ2つのQuadTree間で衝突判定

* 今管理範囲を整数(Rect)にしているが小数版も作るべき?

* コメント&ドキュメントの整備

## 議論の対象
* 数値型の種類
* "四分木を走査し衝突可能性のあるオブジェクト組を列挙"について
  * 今の実装 : for(全ノード) for(そのノードの親)
  * 参考サイトの実装 : スタック使ってDFS
    * ↑だと子空間が空のとき枝刈りできるみたいなことを言ってるが本当か？(自分はわかってない)  
      オブジェクト登録を`O(深さ)`にしていいならわかるが

# サンプル([Main.cpp](https://github.com/falrnd/Sivngi/blob/master/Sivngi/Main.cpp))
<td align="center"><img alt="sample" src="https://user-images.githubusercontent.com/28914324/151736904-2e2d9bf9-3f39-49a3-bcc0-512ec7da7e08.png" ></td>  

```cpp
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
		{
			if (animation)
			{
				objects.each([](auto& e) { e.update(); });
			}

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

			if (SimpleGUI::Button(U"/2", Vec2{ 1090, 20 }, unspecified, objects.size() > 1))
				objects.resize(objects.size() / 2);
			if (SimpleGUI::Button(U"*2", Vec2{ 1160, 20 }))
				objects.resize(objects.size() * 2);
		}
	}
}
```

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
<td align="center"><img alt="sample" src="https://user-images.githubusercontent.com/28914324/151689363-85a67bb9-df49-4469-bde3-469b2b4c1582.png" ></td>  

```cpp
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
```

# サンプル(短め)
```cpp
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

void Main()
{
	Window::Resize(1280, 960);

	//四分木
	auto quadtree = QuadTree<user::Object>{ QuadTreeConfig(6, gamearea) };
	//objects
	Array<user::Object> objects(512);

	int intersectCheckCount = 0;
	int prevClock = 0;

	while (System::Update())
	{
		ClearPrint();
		Print << U"L: 衝突判定したobject組を表示\nSpace: objectの動きを止める";

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

			quadtree(objects)(whenIntersects);
		}

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

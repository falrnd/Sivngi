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


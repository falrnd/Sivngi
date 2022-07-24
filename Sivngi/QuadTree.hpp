#pragma once

#include <Siv3D.hpp>

namespace s3d
{
	// todo
	// * sectionが十分に大きくない場合とかのためにdouble版も作りたい
	// * でも全部doubleにすると速度落ちそう
	// * そもそもObjectを毎回設定しなおす方向性でいいのか
	// * Nodeのメモリをいつ解放するのか(オブジェクトが出入りする度に再確保/解放するのはちょっと)

	/// level: 四分木の層
	/// section: 四分木のあるlevelでの分割された1区画
	template <class Element>
	class QuadTree
	{
	private:
		size_t levels;

		Rect region;

		using Node = Array<std::reference_wrapper<Element>>;
		/// 木を線形に持つ
		Array<Node> linertree;

		[[nodiscard]]
		size_t get(const RectF&) const;

		/// todo
		/// iterator
		class Accessor
		{
		private:
			const QuadTree& qt;

		public:
			SIV3D_NODISCARD_CXX20
			Accessor(const QuadTree& qt) : qt(qt) {}

			using Pred = std::function<void(Element&, Element&)>;

			//走査
			void operator()(const Pred&) const;
		};

	public:
		/// level = 0 -> 通常の全オブジェクト間チェック
		/// level = 1 -> 4分割(縦横2分割)
		/// level = 2 -> 16分割(縦横4分割)
		///
		/// region: 管理する領域全体
		SIV3D_NODISCARD_CXX20
		QuadTree(size_t levels, Rect region);

		[[nodiscard]]
		size_t getLevels() const { return levels; }
		void setLevels(size_t levels);

		[[nodiscard]]
		const Rect& getRegion() const { return region; }
		size_t setRegion(Rect region) { this.region = region; };

		//構築
		[[nodiscard]]
		Accessor operator()(Array<Element>&);

		size_t getMemsizeRough() const;
	};

}

# include "detail/QuadTree.ipp"

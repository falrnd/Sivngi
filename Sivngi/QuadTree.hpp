#pragma once

#include <Siv3D.hpp>

namespace s3d
{
	/**
	* layer: 四分木のn番目の層(0-indexed)
	* section: 四分木のあるlayerでの分割された1区画
	**/

	// todo
	// * sectionが十分に大きくない場合とかのためにdouble版も作りたい
	// * でも全部doubleにすると速度落ちそう
	struct QuadTreeConfig
	{
		/// 最下位空間の番号(0-indexed)
		size_t lowestLayer;

		/// 管理する領域
		Rect gamearea;

		SIV3D_NODISCARD_CXX20
		constexpr QuadTreeConfig(size_t _layers, const Rect& _region = Scene::Rect())
			: gamearea(_region), lowestLayer(_layers - 1)
		{
		}
	};

	/// todo
	/// * configの再設定
	/// * そもそもObjectを毎回設定しなおす方向性でいいのか
	template <class Element>
	class QuadTree
	{
	private:
		QuadTreeConfig config;

		using Node = Array<std::reference_wrapper<Element>>;
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
		SIV3D_NODISCARD_CXX20
		QuadTree(const QuadTreeConfig& config);

		[[nodiscard]]
		const auto& currentConfig() const { return config; }

		//構築
		[[nodiscard]]
		Accessor operator()(Array<Element>&);
	};

}

# include "detail/QuadTree.ipp"

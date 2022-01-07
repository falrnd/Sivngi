#pragma once

#include <Siv3D.hpp>

// http://marupeke296.com/COL_2D_No8_QuadTree.html
// https://qiita.com/Yujiro-Ito/items/1078db2d78f92898b813

namespace s3d
{
	/**
	* layer: 四分木のn番目の層(0-indexed)
	* section: 四分木のあるlayerでの分割された1区画
	**/

	namespace detail
	{
		using I16 = s3d::uint32;//s3d::uint16;
		using I32 = s3d::uint32;
	}

	// todo
	// * sectionが十分に大きくない場合とかのためにdouble版も作りたい
	// * でも全部doubleにすると速度落ちそう
	struct QuadTreeConfig
	{
		/// 最下位空間の番号(0-indexed)
		size_t lowestLayer;

		/// 管理する領域
		Rect region;

		[[nodiscard]] constexpr QuadTreeConfig(size_t _layers, const Rect& _region = Scene::Rect())
			: region(_region), lowestLayer(_layers - 1)
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

		size_t get(const RectF&) const;

		/// todo
		/// iterator
		class Accessor
		{
		private:
			const QuadTree& qt;

		public:
			Accessor(const QuadTree& qt) : qt(qt) {}

			using Pred = std::function<void(Element&, Element&)>;

			//走査
			void operator()(const Pred&) const;
		};

	public:
		[[nodiscard]] QuadTree(const QuadTreeConfig& config);

		const auto& currentConfig() const { return config; }

		//構築
		Accessor operator()(Array<Element>&);
	};

}

# include "detail/QuadTree.ipp"

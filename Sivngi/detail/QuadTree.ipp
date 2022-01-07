namespace s3d {
	namespace detail
	{
		using I16 = s3d::uint32;//s3d::uint16;
		using I32 = s3d::uint32;

		// (4^L - 1) / (4 - 1)
		// (2^(L*2) - 1) / 3
		[[nodiscard]] constexpr size_t BeginLinertree(size_t layer)
		{
			return ((1ull << (layer * 2)) - 1) / 3;
		}

		[[nodiscard]] constexpr I32 BitSeparate32(I16 n)
		{
			n = (n | (n << 8)) & 0x00ff00ff;
			n = (n | (n << 4)) & 0x0f0f0f0f;
			n = (n | (n << 2)) & 0x33333333;
			return (n | (n << 1)) & 0x55555555;
		}

		[[nodiscard]] constexpr I32 MortonNumber(const Rect& region, const Point& sectionSize, const Point& p)
		{
			const I16 x = static_cast<I16>(Clamp(p.x, 0, region.w - 1) / sectionSize.x);
			const I16 y = static_cast<I16>(Clamp(p.y, 0, region.h - 1) / sectionSize.y);
			Math::Map(p.x, region.pos.x, region.tr().x, 0, sectionSize.x);

			return BitSeparate32(x) | (BitSeparate32(y) << 1);
		}

		//[[nodiscard]] constexpr I32 MortonNumber(const RectF& region, const Vec2& sectionSize, const Point& p)
		//{
		//}

		[[nodiscard]] constexpr size_t GetLayer(size_t lowestLayer, I32 mortonxor)
		{
			const auto y = mortonxor & 0xaaaaaaaa;
			const auto x = mortonxor & 0x55555555;

			//0b?0?0?0
			const auto t = y | (x << 1);
			return lowestLayer - std::bit_width(t) / 2;
		}
	}

	template<class Element>
	size_t QuadTree<Element>::get(const RectF& r) const
	{
		const auto sectionsInRow = 1 << config.lowestLayer;
		// 切り上げ
		const Point sectionSize = config.region.size.movedBy(sectionsInRow - 1, sectionsInRow - 1) / sectionsInRow;

		const detail::I32 mortontl = detail::MortonNumber(config.region, sectionSize, r.tl().asPoint() - config.region.pos);
		const detail::I32 mortonbr = detail::MortonNumber(config.region, sectionSize, r.br().asPoint() - config.region.pos);
		const size_t layer = detail::GetLayer(config.lowestLayer, mortontl ^ mortonbr);
		return detail::BeginLinertree(layer) + (static_cast<size_t>(mortonbr) >> ((config.lowestLayer - layer) * 2));
	}

	template<class Element>
	QuadTree<Element>::QuadTree(const QuadTreeConfig& config)
		: config(config)
		, linertree(detail::BeginLinertree(config.lowestLayer + 1))
	{
	}

	template<class Element>
	QuadTree<Element>::Accessor QuadTree<Element>::operator()(Array<Element>& elements)
	{
		for (auto&& node : linertree)
			node.clear();

		for (auto& e : elements)
		{
			//todo: boundingRectを得る関数は外部から与えるべき？
			linertree[get(e.boundingRect())].emplace_back(e);
			//linertree[config.GetIndexInLinertree(e)].emplace_back(e);
		}

		return { *this };
	}

	namespace detail {

	}

	template<class Element>
	void QuadTree<Element>::Accessor::operator()(const QuadTree<Element>::Accessor::Pred& f) const
	{
		//layerをなめる
		for (int layer1 = 0; layer1 <= qt.config.lowestLayer; ++layer1)
		{
			const size_t beginLt1 = detail::BeginLinertree(layer1);
			//layer内のsectionをなめる
			for (size_t morton = 0, sectionsInLayer = 1ull << (layer1 * 2); morton < sectionsInLayer; ++morton)
			{
				auto&& node = qt.linertree[beginLt1 + morton];
				if (!node)
					continue;

				for (size_t i = 0, e = node.size(); i < e; ++i)
					for (size_t k = i + 1; k < e; ++k)
						f(node[i], node[k]);

				size_t morton2 = morton >> 2;
				//sectionの親空間をなめる
				for (int layer2 = layer1 - 1; layer2 >= 0; --layer2, morton2 >>= 2)
				{
					auto&& node2 = qt.linertree[detail::BeginLinertree(layer2) + morton2];
					if (!node2)
						continue;

					for (const auto& a : node)
						for (const auto& b : node2)
							f(a, b);
				}
			}
		}
	}
}

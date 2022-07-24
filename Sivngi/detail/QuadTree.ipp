namespace s3d {
	namespace detail
	{
		// (4^L - 1) / (4 - 1)
		// (2^(L*2) - 1) / 3
		[[nodiscard]] constexpr size_t BeginLinertree(size_t level)
		{
			return ((1ull << (level * 2)) - 1) / 3;
		}

		[[nodiscard]] constexpr s3d::uint32 BitSeparate32(s3d::uint16 n)
		{
			n = (n | (n << 8)) & 0x00ff00ff;
			n = (n | (n << 4)) & 0x0f0f0f0f;
			n = (n | (n << 2)) & 0x33333333;
			return (n | (n << 1)) & 0x55555555;
		}

		[[nodiscard]] constexpr s3d::uint32 MortonNumber(const Rect& gamearea, const Point& sectionSize, const Point& p)
		{
			const auto x = static_cast<s3d::uint16>(Clamp(p.x, 0, gamearea.w - 1) / sectionSize.x);
			const auto y = static_cast<s3d::uint16>(Clamp(p.y, 0, gamearea.h - 1) / sectionSize.y);

			// for double
			// この後整数にキャストしたいがMapって半開区間でも大丈夫なのか?
			// auto nx = Math::Map(p.x, gamearea.tl().x, gamearea.br().x, 0, sectionSize.x);
			// auto ny = Math::Map(p.y, gamearea.tl().y, gamearea.br().y, 0, sectionSize.y);

			return BitSeparate32(x) | (BitSeparate32(y) << 1);
		}

		[[nodiscard]] constexpr size_t GetLayer(size_t lowestLevel, s3d::uint32 mortonxor)
		{
			const auto y = mortonxor & 0xaaaaaaaa;
			const auto x = mortonxor & 0x55555555;

			//0b?0?0?0
			const auto t = y | (x << 1);
			return lowestLevel - std::bit_width(t) / 2;
		}
	}

	template<class Element>
	size_t QuadTree<Element>::get(const RectF& r) const
	{
		const auto sectionsInRow = 1 << config.lowestLevel;
		// 切り上げ
		const Point sectionSize = config.region.size.movedBy(sectionsInRow - 1, sectionsInRow - 1) / sectionsInRow;

		const s3d::uint32 mortontl = detail::MortonNumber(config.region, sectionSize, r.tl().asPoint() - config.region.pos);
		const s3d::uint32 mortonbr = detail::MortonNumber(config.region, sectionSize, r.br().asPoint() - config.region.pos);
		const size_t layer = detail::GetLayer(config.lowestLevel, mortontl ^ mortonbr);
		return detail::BeginLinertree(layer) + (static_cast<size_t>(mortonbr) >> ((config.lowestLevel - layer) * 2));
	}

	template<class Element>
	QuadTree<Element>::QuadTree(const QuadTreeConfig& config)
		: config(config)
		, linertree(detail::BeginLinertree(config.lowestLevel + 1))
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

	template<class Element>
	void QuadTree<Element>::Accessor::operator()(const QuadTree<Element>::Accessor::Pred& f) const
	{
		//layerをなめる
		for (int level1 = 0; level1 <= qt.config.lowestLevel; ++level1)
		{
			const size_t beginLt1 = detail::BeginLinertree(level1);
			//layer内のsectionをなめる
			for (size_t morton = 0, sectionsInLayer = 1ull << (level1 * 2); morton < sectionsInLayer; ++morton)
			{
				auto&& node = qt.linertree[beginLt1 + morton];
				if (!node)
					continue;

				for (size_t i = 0, e = node.size(); i < e; ++i)
					for (size_t k = i + 1; k < e; ++k)
						f(node[i], node[k]);

				size_t morton2 = morton >> 2;
				//sectionの親空間をなめる
				for (int level2 = level1 - 1; level2 >= 0; --level2, morton2 >>= 2)
				{
					auto&& node2 = qt.linertree[detail::BeginLinertree(level2) + morton2];
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

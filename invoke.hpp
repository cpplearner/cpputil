template<class F, class... ArgTypes>
auto invoke(F&& f, ArgTypes&&... args);

template<class R, class F, class... ArgTypes>
R invoke(F&& f, ArgTypes&&... args);

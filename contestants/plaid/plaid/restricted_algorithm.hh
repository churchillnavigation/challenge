#ifndef RESTRICTED_ALGORITHM_HH
#define RESTRICTED_ALGORITHM_HH

template<class ItA, class ItD, class Trans>
ItD transform_into(
    ItA a_src, ItA a_lim,
    ItD d_src,
    Trans trans
) {
    while(a_src != a_lim)
    {
        trans(*a_src, *d_src);
        ++a_src;
        ++d_src;
    }
    return d_src;
}

template<class ItA, class ItD>
__attribute__((hot))
ItD restricted_copy(ItA a_src, ItA a_lim, ItD d_src, ItD d_lim)
{
    if((a_lim - a_src) < (d_lim - d_src))
    {
        while(a_src != a_lim)
        {
            *d_src = *a_src;
            ++a_src;
            ++d_src;
        }
    }
    else
    {
        while(d_src != d_lim)
        {
            *d_src = *a_src;
            ++a_src;
            ++d_src;
        }
    }
    return d_src;
}

template<class ItA, class ItD, class Filt>
__attribute__((hot))
ItD restricted_copy_if(ItA a_src, ItA a_lim, ItD d_src, ItD d_lim, Filt filt)
{
    while(a_src != a_lim && d_src != d_lim)
    {
        if(filt(*a_src))
        {
            *d_src = *a_src;
            ++d_src;
        }

        ++a_src;
    }

    return d_src;
}

template<class ItA, class ItD, class Trans>
ItD restricted_transform(
    ItA a_src, ItA a_lim,
    ItD d_src, ItD d_lim,
    Trans trans
) {
    while(a_src != a_lim && d_src != d_lim)
    {
        *d_src = trans(*a_src);
        ++a_src;
        ++d_src;
    }

    return d_src;
}

template<class ItA, class ItD, class Trans, class Filt>
ItD restricted_transform_if(
    ItA a_src, ItA a_lim,
    ItD d_src, ItD d_lim,
    Trans trans,
    Filt filt
) {
    while(a_src != a_lim && d_src != d_lim)
    {
        if(filt(*a_src))
        {
            *d_src = trans(*a_src);
            ++d_src;
        }

        ++a_src;
    }

    return d_src;
}

template<class ItA, class ItB, class ItD, class Comp, class Filt>
__attribute__((hot))
ItD restricted_merge_if(
    ItA a_src, ItA a_lim,
    ItB b_src, ItB b_lim,
    ItD d_src, ItD d_lim,
    Comp comp,
    Filt filt
) {
 filt:
    while(true)
    {
        if(a_src == a_lim)
        {
            goto degenerate_a;
        }
        if(filt(*a_src))
        {
            goto cmp;
        }
        ++a_src;
    }

 cmp:
    while((b_src != b_lim) && (d_src != d_lim))
    {
        if(comp(*b_src, *a_src))
        {
            *d_src = *b_src;
            ++d_src;
            ++b_src;
        }
        else
        {
            *d_src = *a_src;
            ++d_src;
            ++a_src;

            goto filt;
        }
    }

    // Remember, the filter is only supposed to be applied to sequence A, so we
    // don't use the *if variant here.
    d_src = restricted_copy_if(a_src, a_lim, d_src, d_lim, filt);
 degenerate_a:
    return restricted_copy(b_src, b_lim, d_src, d_lim);
}

template<class ItA, class ItB, class ItD, class Filt>
ItD restricted_merge_if(
    ItA a_src, ItA a_lim,
    ItB b_src, ItB b_lim,
    ItD d_src, ItD d_lim,
    Filt filt
) {
    return restricted_merge_if(
        a_src,
        a_lim,
        b_src,
        b_lim,
        d_src,
        d_lim,
        [](decltype(*a_src) a, decltype(*b_src) b){return a < b;},
        filt
    );
}

template<class ItA, class ItB, class ItD, class Comp>
ItD restricted_merge(
    ItA a_src, ItA a_lim,
    ItB b_src, ItB b_lim,
    ItD d_src, ItD d_lim,
    Comp comp
) {
    return restricted_merge_if(
        a_src,
        a_lim,
        b_src,
        b_lim,
        d_src,
        d_lim,
        comp,
        [](decltype(*a_src)){return true;}
    );
}

template<class ItA, class ItB, class ItD>
ItD restricted_merge(
    ItA a_src, ItA a_lim,
    ItB b_src, ItB b_lim,
    ItD d_src, ItD d_lim
) {
    return restricted_merge(
        a_src, a_lim, b_src, b_lim, d_src, d_lim,
        [](decltype(*a_src) a, decltype(*b_src) b){return a < b;}
    );
}

#endif

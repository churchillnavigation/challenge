function top = rectn(d, lx, hx, ly, hy)

top = d(find(and(and(d(:,2)>=lx,d(:,2)<=hx),and(d(:,3)>=ly,d(:,3)<=hy))),:);

end


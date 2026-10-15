# Rank badge atlas

Generated with the built-in image-generation tool, using the approved concept sheet
and its first transparent extraction as visual references. No game artwork was
replaced. Original concept: ../concepts/rank-symbols-v1.png.

Final edit prompt:

Use case: precise-object-edit. Edit target: provided transparent 22-badge rank atlas. Preserve the exact 22 badge designs, colors, metallic materials, central puck cores and division lights. Change ONLY spacing and scale for a usable game UI sprite atlas. Transparent RGBA canvas, 3 columns and 8 equally spaced rows, no text. Each cell has identical dimensions. Shrink every badge to fit fully within 70% of its cell width AND 70% of its cell height, centered in each cell. Wide red Overdrive badges MUST have generous empty transparent space separating them. No touching or overlap between any badges or glows. Row1 Carbon I II III. Row2 Alloy I II III. Row3 Tempered I II III. Row4 Vector I II III. Row5 Prism I II III. Row6 Overdrive I II III. Row7 Singularity I II III. Row8 only Zenith in middle cell, same visual height as others, left/right empty. All eight row center spacings identical, Zenith must NOT be larger than other badges. Keep each badge entirely isolated. No background, no labels, no new details.

The delivered image uses explicit per-badge UV regions in SFlickRankBadge rather
than assuming that generative artwork has a mathematically exact grid.

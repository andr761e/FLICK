# Profile overview artwork — V2

Generated with the built-in image-generation tool on 2026-10-23. These are text-free decorative images, not live arena replacements. Actual profile identity, equipped puck, stats, rank badges and navigation remain native Slate.

References: `Deliverables/UI-Visual-Upgrade-2026-10-22/Concepts/07-player-profile.png`; the customization illustration also uses `AssetDevelopment/UI/PuckThumbnails/Presentation/Standard_0.png` as its puck-model reference.

Final masters (1672 × 941) are stored alongside this file. Runtime textures use BC7/sRGB with streamed mips, 2048 max for the backdrop and 1024 max for card illustrations. Reimport with `Tools/Import-FlickMenuArtwork.py -FlickProfileArtwork` through Unreal's Python commandlet.

## ProfileBackdropV2.png

```text
Use case: stylized-concept
Asset type: text-free full-screen background for FLICK player profile UI, landscape 16:9.
Input image 1: style and arena composition reference only, not an edit target.
Primary request: Generate ONLY the cinematic empty arena background visible behind the reference profile interface. Remove all interface panels, logos, text, cards, buttons, avatars, illustrations and foreground pucks. This is a usable background plate, not a UI mockup.
Scene: a premium small circular tabletop competitive puck arena in a dark stadium. Camera looking diagonally down across dark brushed graphite steel playing surface with subtle concentric rings and a slim cyan and amber segmented rim in the bottom foreground. Warm amber stadium lamps across the upper edge and cool cyan rail lights, realistic restrained floor reflections, soft depth of field. Keep the floor physically grounded and uncluttered.
Composition: arena occupies the frame, stadium lighting horizon around upper fifth, broad dark quiet surface behind native menu cards in middle and lower portions. Match the cool navy/black, amber and cyan atmosphere of the reference, not a pale blue arena. High-quality photorealistic game-environment render.
Constraints: NO text, NO logo, NO UI, NO badges, NO pucks, NO people, NO watermark; single seamless scene, no montage or panel borders. Landscape 16:9.
```

## ProfileRankStageV2.png

```text
Use case: stylized-concept
Asset type: text-free decorative illustration for the bottom-right of a FLICK competitive-rank menu card, landscape 16:9.
Input image 1: style and placement reference only.
Primary request: Generate a dark premium navy-black illustration of an EMPTY low circular graphite metal podium with subtle cyan luminous edge, like the small rank platform in the reference's Competitive Ranks card, but WITHOUT the floating question-mark disc or any rank badge. We will draw the real player's badge in native UI on top.
Composition: podium at approximately 70 percent across, 76 percent down; occupies right half and bottom third; faint empty metallic shield outlines behind the podium on right side only. Left half and top third very dark navy negative space with only faint diagonal cyan/amber floor streaks. Smoothly darken all edges into almost-black navy so this art blends into a card.
Style: polished physically grounded 3D game product render, brushed graphite, silver edge highlights, restrained cyan illumination and tiny amber reflections. Same palette and material language as reference.
Constraints: NO text, NO numerals, NO question mark, NO badge sitting or floating on stage, NO logo, NO UI, NO border, NO watermark. Single illustration, not a UI mockup. Landscape 16:9.
```

## ProfileCustomizeV2.png

```text
Use case: product-mockup
Asset type: text-free decorative illustration for the lower-right of a FLICK Customize profile card, landscape 16:9.
Input image 1: overall style/layout reference only. Input image 2: actual in-game Standard puck model shape/material reference.
Primary request: Generate a premium render of THREE recognizable Standard FLICK pucks, with the same segmented silver upper rim, graphite cylindrical body, circular ring symbol and slim luminous lower strip as input 2. Cyan-blue puck front left, amber puck behind in middle, amethyst-purple puck behind on right. Slightly overlapping fan arrangement, blue puck slightly tilted to show its upper face, like the customization-card illustration in input 1.
Composition: three pucks grouped in the right two-thirds and lower half of a landscape image on a reflective dark navy steel surface. Leave left third and top third dark for native interface text. Smoothly fade outer edges into almost-black navy.
Lighting: cinematic cyan and warm amber reflections, polished brushed metal, realistic shadows, premium but restrained glow, same dark stadium atmosphere as reference.
Constraints: Only these three pucks, NO text, NO labels, NO logos, NO UI, NO border, NO watermark. Do not add effects or redesign the pucks. Landscape 16:9.
```


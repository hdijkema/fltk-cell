# fltk-cell development

## Library website

`docs/index.html` is the standalone landing page. `docs/assets/logo.svg` is the
library's vector logo and favicon; `docs/style.css` provides responsive layout
and explicit light/dark themes. All assets and fonts are local or system-provided;
the only script is the small theme control; the library itself has no website dependency.

GitHub Pages can publish the default branch's `/docs` directory. Jekyll uses
`docs/_config.yml`, `jekyll-optional-front-matter` and the documentation layout
to render `API.md` and this guide as HTML. Repository-relative source links in
the Markdown are mapped to GitHub by the layout. Documentation stays owned by
this repository. The landing illustration is an HTML/CSS diagram, not a product
screenshot. The logo uses the same rounded-square geometry as the other widget
libraries, with a distinct symbol and accent color.

To build locally with Jekyll 3.10 and `kramdown-parser-gfm` installed:

```sh
jekyll build --source docs --destination build/site/fltk-cell
python -m http.server 8000 --directory build/site
```

Open `http://localhost:8000/fltk-cell/`. The destination includes the configured
repository base path so API-page assets resolve correctly. Publishing requires the repository's GitHub Pages setting;
adding these source files does not change that setting or publish the site.

## Website theme control

The homepage and documentation layout start with `data-theme="dark"`.
`theme.js` restores a saved light/dark choice before the body paints and attaches
the keyboard-accessible header button. The preference uses the site's own
`data-theme-key` in local storage; storage failures do not prevent switching.
The default is dark even when the operating system prefers light. Without
JavaScript, the pages retain their default dark appearance.

## API documentation order and library navigation

`API.md` starts with the public API's construction, ownership and behavior,
followed by a complete program. Detailed class/member references and usage
patterns precede supporting public data types and build details. Keep complete
examples self-contained when updating the API.

The homepage and documentation header include a native HTML `details` dropdown
before the theme button, linking to the other four library homepages. It uses
ordinary links and works without JavaScript. Update both headers when the
library list changes.

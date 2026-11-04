import io
import logging
from concurrent.futures import Future, ThreadPoolExecutor
from dataclasses import dataclass, field
from typing import Annotated, Literal

import mercantile
import shapely
from fastapi import Body, Depends, FastAPI, Path, Query, Response
from fastapi.middleware.cors import CORSMiddleware
from geojson_pydantic.geometries import MultiPolygon, Polygon
from PIL import Image
from pydantic import AnyUrl, BaseModel, Field
from pystac import Link
from pystac_client import Client
from rio_tiler.colormap import cmap
from rio_tiler.io import STACReader
from rio_tiler.models import ImageData
from rio_tiler.mosaic import mosaic_reader
from rio_tiler.utils import create_cutline
from shapely.geometry import shape

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
logger.addHandler(logging.StreamHandler())

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@dataclass
class Searcher:
    base: str = field(default="https://earth-search.aws.element84.com/v1")

    _client: Client = field(init=False)

    def __post_init__(self):
        self._client = Client.open(self.base)

    def search(self, geometry: dict, collection: str, limit: int = 1):
        return self._client.search(
            max_items=limit,
            intersects=geometry,
            collections=[collection],
            query={"eo:cloud_cover": {"lt": 10}},
            sortby=[{"field": "properties.datetime", "direction": "desc"}],
        )

    def url(self, geometry: dict, collection: str, limit: int = 1):
        result = self.search(geometry, collection, limit=limit)
        # Only the `self` link points at the STAC Item itself; the rest are
        # catalog/collection/asset links that STACReader cannot open.
        return [
            link for item in result.items() for link in item.links if link.rel == "self"
        ]


def empty() -> bytes:
    with Image.new("RGBA", (1, 1), (0, 0, 0, 0)) as img, io.BytesIO() as buf:
        img.save(buf, format="PNG")
        return buf.getvalue()


class PostTileBody(BaseModel):
    x: Annotated[int, Field()]
    y: Annotated[int, Field()]
    z: Annotated[int, Field()]
    fmt: Annotated[Literal["png"], Field(default="png")]
    geometries: Annotated[list[Polygon | MultiPolygon], Field(default_factory=list)]
    assets: Annotated[list[str], Field(default_factory=lambda: ["visual"])]
    rescale: Annotated[list[list[float]], Field(default=[[0, 255]])]
    expr: Annotated[str | None, Field(default=None)]
    colormap: Annotated[str | None, Field(default=None)]
    collection: Annotated[str, Field(default="sentinel-2-l2a")]


@dataclass
class PostTile:
    body: PostTileBody
    searcher: Annotated[Searcher, Depends(lambda: Searcher())]


@app.post("/tiles")
def post_tile(ctx: Annotated[PostTile, Depends(PostTile)]):
    tile = shape(mercantile.feature((ctx.body.x, ctx.body.y, ctx.body.z)))

    if not ctx.body.geometries:
        return Response(content=empty(), media_type="image/png")

    geometries = [shape(geom) for geom in ctx.body.geometries]
    geometries = shapely.union_all(geometries)

    if not tile.intersects(geometries):
        return Response(content=empty(), media_type="image/png")

    geometry = geometries.intersection(tile)
    # raise limit for automatic mosaic reader (makes the render very slow)
    curls = ctx.searcher.url(
        geometry.__geo_interface__,
        collection=ctx.body.collection,
        limit=1,
    )
    logger.debug(f"Total URLs: {len(curls)}")
    for curl in curls:
        logger.debug(f"URL: {curl.href}")

    def read(curl: Link) -> ImageData:
        with STACReader(curl.href) as reader:
            return reader.tile(
                ctx.body.x,
                ctx.body.y,
                ctx.body.z,
                assets=ctx.body.assets,
                expression=ctx.body.expr,
                vrt_options={
                    "cutline": create_cutline(
                        src_dst=reader,
                        geometry=geometry.__geo_interface__,
                        geometry_crs="epsg:4326",
                    )
                },
            )

    image, _ = mosaic_reader(curls, read)

    if ctx.body.rescale:
        image = image.post_process(in_range=ctx.body.rescale)

    colormap = cmap.get(ctx.body.colormap) if ctx.body.colormap else None
    content = image.render(img_format="PNG", colormap=colormap)

    return Response(content=content, media_type="image/png")


@dataclass
class PostImageBody(BaseModel):
    href: Annotated[AnyUrl, Field()]
    geometry: Annotated[Polygon | MultiPolygon, Field()]
    collection: Annotated[str, Field(default="sentinel-2-l2a")]
    fmt: Annotated[Literal["png"], Field(default="png")]
    assets: Annotated[list[str], Field(default_factory=lambda: ["visual"])]
    rescale: Annotated[list[list[float]], Field(default=[[0, 255]])]
    expr: Annotated[str | None, Field(default=None)]
    colormap: Annotated[str | None, Field(default=None)]


@dataclass
class PostImage:
    body: PostImageBody
    searcher: Annotated[Searcher, Depends(lambda: Searcher())]


@app.post("/image")
def get_image(ctx: Annotated[PostImage, Depends(PostImage)]):
    url = ctx.body.href.encoded_string()

    with STACReader(url) as reader:
        image = reader.feature(
            ctx.body.geometry.__geo_interface__,
            assets=ctx.body.assets,
            expression=ctx.body.expr,
            vrt_options={
                "cutline": create_cutline(
                    src_dst=reader,
                    geometry=ctx.body.geometry.__geo_interface__,
                    geometry_crs="epsg:4326",
                )
            },
        )

    if ctx.body.rescale:
        image = image.post_process(in_range=ctx.body.rescale)

    colormap = cmap.get(ctx.body.colormap) if ctx.body.colormap else None
    content = image.render(img_format="PNG", colormap=colormap)

    return Response(content=content, media_type="image/png")


@dataclass
class PostSearchBody(BaseModel):
    geometries: Annotated[list[Polygon | MultiPolygon], Field(default_factory=list)]


@dataclass
class PostSearch:
    body: PostSearchBody
    searcher: Annotated[Searcher, Depends(lambda: Searcher())]


@app.post("/search")
def post_search(ctx: Annotated[PostSearch, Depends(PostSearch)]):
    logger.debug(f"Total geometries: {len(ctx.body.geometries)}")

    geometry = [shape(geom) for geom in ctx.body.geometries]
    geometry = shapely.union_all(geometry)

    res = ctx.searcher._client.search(
        max_items=10,
        intersects=geometry.__geo_interface__,
        collections=["sentinel-2-l2a"],
        query={"eo:cloud_cover": {"lt": 10}},
        sortby=[{"field": "properties.datetime", "direction": "asc"}],
    )

    response = ctx.searcher.url(
        geometry=geometry.__geo_interface__,
        collection="sentinel-2-l2a",
        limit=10,
    )

    return [i.absolute_href for i in response]



@dataclass
class PostSql:
    body: Annotated[list[Polygon | MultiPolygon], Body()]
    query: Annotated[str, Query()]
    searcher: Annotated[Searcher, Depends(lambda: Searcher())]


@app.post("/sql")
def post_sql(ctx: Annotated[PostSql, Depends(PostSql)]):
    logger.debug(f"Total geometries: {len(ctx.body)}")
    logger.debug(f"Query: {ctx.query}")

    futures: list[Future[list[Link]]] = []
    with ThreadPoolExecutor() as executor:
        for geometry in ctx.body:
            futures.append(
                executor.submit(
                    ctx.searcher.url,
                    geometry.__geo_interface__,
                    collection="sentinel-2-l2a",
                    limit=1,
                )
            )

    for geom, future in zip(ctx.body, futures):
        result = future.result()
        logger.debug(
            f"Geometry {geom} matched {len(result)} items: {[i.href for i in result]}"
        )

    return {"query": ctx.query}

import io
import logging
from dataclasses import dataclass, field
from typing import Annotated, Literal

import mercantile
import shapely
from fastapi import Depends, FastAPI, Response
from fastapi.middleware.cors import CORSMiddleware
from geojson_pydantic.geometries import MultiPolygon, Polygon
from PIL import Image
from pydantic import BaseModel
from pystac_client import Client
from rio_tiler.colormap import cmap
from rio_tiler.io import STACReader
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

    def search(self, geometry: dict):
        return self._client.search(
            limit=1,
            intersects=geometry,
            collections=["sentinel-2-l2a"],
            query={"eo:cloud_cover": {"lt": 10}},
            sortby=[{"field": "properties.datetime", "direction": "asc"}],
        )

    def url(self, geometry: dict):
        result = self.search(geometry)
        for item in result.items():
            for link in item.links:
                if link.rel == "self":
                    return link.href
        raise ValueError("No item found")


def empty() -> bytes:
    with Image.new("RGBA", (1, 1), (0, 0, 0, 0)) as img, io.BytesIO() as buf:
        img.save(buf, format="PNG")
        return buf.getvalue()


class PostTileBody(BaseModel):
    x: int
    y: int
    z: int
    fmt: Literal["png"] = "png"
    geometries: list[Polygon | MultiPolygon] = []
    assets: list[str] = ["red", "green", "blue"]
    rescale: list[list[float]] = [(0, 2048), (0, 2048), (0, 2048)]
    expr: str | None = None
    colormap: str | None = None


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
    curl = ctx.searcher.url(geometry.__geo_interface__)
    logger.debug(f"URL: {curl}")

    with STACReader(curl) as reader:
        image = reader.tile(
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

        if ctx.body.rescale:
            image = image.post_process(in_range=ctx.body.rescale)

        colormap = cmap.get(ctx.body.colormap)
        content = image.render(img_format="PNG", colormap=colormap)

    return Response(content=content, media_type="image/png")

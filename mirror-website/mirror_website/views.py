import json

from django.conf import settings
from django.http import (
    HttpRequest,
    HttpResponse,
    HttpResponseBadRequest,
    HttpResponseForbidden,
    HttpResponseNotFound,
)
from django.shortcuts import render
from django.views.decorators.cache import cache_page
from django.views.decorators.http import require_GET, require_http_methods
from .utils import getMirrorsJson, getSyncTokensJson

import logging

_logger = logging.getLogger("mirror-website")


@cache_page(30)
@require_GET
def home(request: HttpRequest):
    return render(request, "home.html")


@cache_page(30)
@require_GET
def projects(request: HttpRequest):
    mirrors_json = getMirrorsJson()
    distributions: dict[str, str] = {}
    software: dict[str, str] = {}
    miscellaneous: dict[str, str] = {}
    for name, mirror in mirrors_json["mirrors"].items():
        match mirror.get("page", ""):
            case "Distributions":
                distributions[name] = mirror
            case "Software":
                software[name] = mirror
            case _:
                miscellaneous[name] = mirror
    context = {
        "distributions": distributions,
        "software": software,
        "miscellaneous": miscellaneous,
    }
    return render(request, "projects.html", context)


@cache_page(30)
@require_GET
def about(request: HttpRequest):
    return render(request, "about.html")


@cache_page(30)
@require_GET
def map(request: HttpRequest):
    return render(request, "map.html")


@cache_page(30)
@require_GET
def contact(request: HttpRequest):
    return render(request, "contact.html")

@cache_page(30)
@require_http_methods(["GET", "POST"])
def sync(request: HttpRequest, project: str):
    token = str()
    if request.method == "GET":
        token = request.GET.get("token", "")
    else:
        token = request.POST.get("token", "")
    if token == "":
        return HttpResponseBadRequest("Bad Request")
    tokens_json = getSyncTokensJson()
    project_token = tokens_json.get(project)
    if project_token is None:
        return HttpResponseNotFound("Not Found")
    if token != project_token:
        return HttpResponseForbidden("Forbidden")
    _logger.info("Manual sync requested for project %s", project)

    # TODO implement manual sync

    return HttpResponse("OK")

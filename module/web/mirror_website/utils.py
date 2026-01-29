import json

from django.conf import settings


def getMirrorsJson() -> dict[str, dict[str, str]]:
    with open(settings.MIRRORS_FILE, "rb") as f:
        return json.loads(f.read())


def getSyncTokensJson() -> dict[str, str]:
    with open(settings.SYNC_TOKENS_FILE, "rb") as f:
        return json.loads(f.read())

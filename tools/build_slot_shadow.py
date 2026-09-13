"""Build a separate contact/soft shadow layer, leaving the armor artwork untouched."""
from PIL import Image, ImageChops, ImageFilter


def separate_shadow(overlay):
    alpha = overlay.convert('RGBA').getchannel('A')
    # Tight contact shading retains the ornament silhouette; a wider soft edge
    # separates the cloth and metal from the background without a hard outline.
    contact = alpha.filter(ImageFilter.MaxFilter(3)).filter(ImageFilter.GaussianBlur(1.5))
    ambient = alpha.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.GaussianBlur(5))
    contact = contact.point(lambda a: round(a * .85))
    ambient = ambient.point(lambda a: round(a * .5))
    shade = Image.new('RGBA', overlay.size)
    shade.putalpha(ImageChops.screen(contact, ambient))
    return shade

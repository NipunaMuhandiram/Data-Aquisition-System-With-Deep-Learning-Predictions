"""
ASGI config for sensor_monitoring project.

It exposes the ASGI callable as a module-level variable named ``application``.

For more information on this file, see
https://docs.djangoproject.com/en/5.1/howto/deployment/asgi/
"""

# import os

# from django.core.asgi import get_asgi_application

# os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'sensor_monitoring.settings')

# application = get_asgi_application()




# sensor_monitoring/asgi.py

import os
from django.core.asgi import get_asgi_application
from channels.routing import ProtocolTypeRouter, URLRouter
from channels.auth import AuthMiddlewareStack
from sensors import routing

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'sensor_monitoring.settings')

application = ProtocolTypeRouter({
    "http": get_asgi_application(),
    "websocket": AuthMiddlewareStack(
        URLRouter(
            routing.websocket_urlpatterns
        )
    ),
})


# import os

# from django.core.asgi import get_asgi_application
# from channels.routing import ProtocolTypeRouter # <- Add this
# from channels.auth import AuthMiddlewareStack # <- Add this
# from sensors.consumers import PresenceConsumer # <- Add this

# os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'sensor_monitoring.settings')

# application = ProtocolTypeRouter(
#     {
#         "http": get_asgi_application(),
#         "websocket": AuthMiddlewareStack(
#             PresenceConsumer.as_asgi()
#         ),
#     }
# )
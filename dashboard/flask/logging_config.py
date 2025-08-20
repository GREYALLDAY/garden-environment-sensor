import os
import logging
from logging.handlers import TimedRotatingFileHandler
from settings import LOG_DIR

os.makedirs(LOG_DIR, exist_ok=True)

def setup_loggers(app):
    formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')

    # Console handler
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    console_handler.setFormatter(formatter)

    # General log handlers
    debug_handler = _create_handler('debug.log', logging.DEBUG, formatter)
    info_handler = _create_handler('info.log', logging.INFO, formatter)
    error_handler = _create_handler('error.log', logging.ERROR, formatter)

    app.logger.setLevel(logging.DEBUG)
    app.logger.addHandler(console_handler)
    app.logger.addHandler(debug_handler)
    app.logger.addHandler(info_handler)
    app.logger.addHandler(error_handler)

    # MQTT logger
    mqtt_logger = logging.getLogger("mqtt")
    mqtt_logger.setLevel(logging.INFO)
    mqtt_logger.addHandler(_create_handler('mqtt.log', logging.INFO, formatter))
    mqtt_logger.propagate = False

    # NTFY logger
    ntfy_logger = logging.getLogger("ntfy")
    ntfy_logger.setLevel(logging.INFO)
    ntfy_logger.addHandler(_create_handler('ntfy.log', logging.INFO, formatter))
    ntfy_logger.propagate = False


def _create_handler(filename, level, formatter):
    handler = TimedRotatingFileHandler(
        os.path.join(LOG_DIR, filename),
        when='midnight',
        interval=1,
        backupCount=7,
        encoding='utf-8'
    )
    handler.setLevel(level)
    handler.setFormatter(formatter)
    return handler

# gunicorn.conf.py

# Logging
loglevel = 'debug'
capture_output = True
accesslog = 'logs/gunicorn-access.log'
errorlog = 'logs/gunicorn-error.log'

# Workers and performance
workers = 2
threads = 4
worker_class = 'gevent'  # or 'uvicorn.workers.UvicornWorker' if using FastAPI

# Binding
bind = '0.0.0.0:8000'

# Daemon (optional)
# daemon = True

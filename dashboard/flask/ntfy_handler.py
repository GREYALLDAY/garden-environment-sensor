# ntfy_handler.py
import os
import requests
import logging
from dotenv import load_dotenv


logger = logging.getLogger("ntfy")
load_dotenv()

NTFY_TOKEN = os.getenv("NTFY_TOKEN")
NTFY_HOST = os.getenv("NTFY_HOST", "").rstrip("/")
NTFY_TOPIC = os.getenv("NTFY_TOPIC", "").rstrip("/")

def send_ntfy_message(message, topic=NTFY_TOPIC, title="Garden Alert", priority="default"):
    
  if not NTFY_HOST or not topic:
    logger.info("[ntfy] Skipping: NTFY_HOST or NTFY_TOPIC not configured.")
    return
    
    
  url = f"{NTFY_HOST}/{topic}"
  headers = {
    "Authorization": f"Bearer {NTFY_TOKEN}",
    "Title": title,
    "Priority": priority
  }

  try:
    ## == [DEBUG] these can be commented out ==
    logger.info(f"[ntfy] Host: {NTFY_HOST}")
    logger.info(f"[ntfy] Sending message to {NTFY_HOST}/{topic}")
    logger.info(f"[ntfy] Sending POST to {url}")
    logger.info(f"[ntfy] Payload: {message}") 
    
    resp = requests.post(url, data=message.encode("utf-8"), headers=headers)
    
    if resp.status_code != 200:
        logger.error(f"[ntfy] Failed with {resp.status_code}: {resp.text}")
    else:
        logger.info(f"[ntfy] Message sent successfully: {resp.status_code}")
    resp.raise_for_status()
  
  except Exception as e:
    logger.error(f"[ntfy] Error: {e}")


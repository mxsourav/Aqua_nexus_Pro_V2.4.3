# Blynk Credentials and Setup

This file keeps the Blynk details used in the current working firmware.

## Current Blynk credentials

- Template ID: `TMPL3uh2Rl7fq`
- Template Name: `Project Flour`
- Auth Token: `YOUR_BLYNK_AUTH_TOKEN`

## Where these are written in code

- `include/config.h`
- `src/main.cpp`

## Virtual pin use

| Virtual Pin | Use |
| --- | --- |
| V0 | Moisture percentage |
| V1 | Water level percentage |
| V2 | Pump status |
| V3 | Manual pump button |
| V4 | Moisture threshold |
| V5 | Pump duty control |
| V6 | Log / status string |
| V7 | Next watering epoch |
| V8 | Schedule interval in hours |

## Important note

- If these credentials are changed in one place, check the other file also and keep them same.
- If using a new Blynk template, update template ID, template name, and auth token before upload.

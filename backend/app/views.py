"""
Flask Documentation:     https://flask.palletsprojects.com/
Jinja2 Documentation:    https://jinja.palletsprojects.com/
Werkzeug Documentation:  https://werkzeug.palletsprojects.com/
This file creates your application.
"""

# from crypt import methods
import site 

from app import app, Config,  mongo, Mqtt
from flask import escape, render_template, request, jsonify, send_file, redirect, make_response, send_from_directory 
from json import dumps, loads 
from werkzeug.utils import secure_filename
from datetime import datetime,timedelta, timezone
from os import getcwd
from os.path import join, exists
from time import time, ctime
from math import floor
 



#####################################
#   Routing for your application    #
#####################################

# 1. Route to set a passcode combination
@app.route('/api/set/combination', methods=['POST'])
def set_combination():
    '''Sets or updates the passcode in the code collection'''
    if request.method == 'POST':
        data = request.form.get('passcode')  # Expecting 'passcode=1234' from ESP32
        if not data:
            return jsonify({"status": "error", "message": "No passcode provided"}), 400
        
        result = mongo.setPasscode(data)
        if result:
            return jsonify({"status": "success", "data": dumps(result)}), 200
        return jsonify({"status": "error", "message": "Failed to set passcode"}), 500

# 2. Route to check a passcode combination
@app.route('/api/check/combination', methods=['POST'])
def check_combination():
    '''Checks if the provided passcode exists in the code collection'''
    if request.method == 'POST':
        data = request.form.get('passcode')  # Expecting 'passcode=1234' from ESP32
        if not data:
            return jsonify({"status": "error", "message": "No passcode provided"}), 400
        
        count = mongo.checkPasscode(data)
        if count > 0:
            return jsonify({"status": "complete", "data": "complete"}), 200
        return jsonify({"status": "failed", "data": "failed"}), 401

# 3. Route to update radar data
@app.route('/api/update', methods=['POST'])
def update_radar():
    '''Inserts new radar data into the radar collection'''
    if request.method == 'POST':
        data = request.get_json()  # Expecting JSON payload
        if not data:
            return jsonify({"status": "error", "message": "No data provided"}), 400
        
        # Add timestamp if not provided
        if 'timestamp' not in data:
            data['timestamp'] = int(time())
        
        result = mongo.insertRadarData(data)
        if result:
            return jsonify({"status": "success", "data": str(result)}), 200
        return jsonify({"status": "error", "message": "Failed to insert data"}), 500

# 4. Route to retrieve reserve data between timestamps
@app.route('/api/reserve/<start>/<end>', methods=['GET'])
def get_reserve(start, end):
    '''Retrieves all radar documents between start and end timestamps'''
    if request.method == 'GET':
        try:
            start_ts = int(start)
            end_ts = int(end)
            if start_ts > end_ts:
                return jsonify({"status": "error", "message": "Start timestamp must be less than end"}), 400
            
            result = mongo.get_reserved(start_ts, end_ts)
            return jsonify({"status": "success", "data": result}), 200
        except ValueError:
            return jsonify({"status": "error", "message": "Invalid timestamp format"}), 400
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)}), 500

# 5. Route to compute average reserve between timestamps
@app.route('/api/avg/<start>/<end>', methods=['GET'])
def get_average_reserve(start, end):
    '''Computes the average reserve value between start and end timestamps'''
    if request.method == 'GET':
        try:
            start_ts = int(start)
            end_ts = int(end)
            if start_ts > end_ts:
                return jsonify({"status": "error", "message": "Start timestamp must be less than end"}), 400
            
            result = mongo.get_avg(start_ts, end_ts)
            return jsonify({"status": "success", "data": result}), 200
        except ValueError:
            return jsonify({"status": "error", "message": "Invalid timestamp format"}), 400
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)}), 500






@app.route('/api/file/get/<filename>', methods=['GET']) 
def get_images(filename):   
    '''Returns requested file from uploads folder'''
   
    if request.method == "GET":
        directory   = join( getcwd(), Config.UPLOADS_FOLDER) 
        filePath    = join( getcwd(), Config.UPLOADS_FOLDER, filename) 

        # RETURN FILE IF IT EXISTS IN FOLDER
        if exists(filePath):        
            return send_from_directory(directory, filename)
        
        # FILE DOES NOT EXIST
        return jsonify({"status":"file not found"}), 404


@app.route('/api/file/upload',methods=["POST"])  
def upload():
    '''Saves a file to the uploads folder'''
    
    if request.method == "POST": 
        file     = request.files['file']
        filename = secure_filename(file.filename)
        file.save(join(getcwd(),Config.UPLOADS_FOLDER , filename))
        return jsonify({"status":"File upload successful", "filename":f"{filename}" })

 


###############################################################
# The functions below should be applicable to all Flask apps. #
###############################################################


@app.after_request
def add_header(response):
    """
    Add headers to both force latest IE rendering engine or Chrome Frame,
    and also tell the browser not to cache the rendered page. If we wanted
    to we could change max-age to 600 seconds which would be 10 minutes.
    """
    response.headers['X-UA-Compatible'] = 'IE=Edge,chrome=1'
    response.headers['Cache-Control'] = 'public, max-age=0'
    return response

@app.errorhandler(405)
def page_not_found(error):
    """Custom 404 page."""    
    return jsonify({"status": 404}), 404




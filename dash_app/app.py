import dash
from dash import Dash, dcc, html, callback, Input, Output
from decimal import Decimal


import boto3
from boto3.dynamodb.conditions import Key, Attr
import pandas as pd
import plotly.express as px
import time

# define the AWS DynamoDB table object
ecg_dev_tab = boto3.resource('dynamodb', region_name='eu-west-1')
table = ecg_dev_tab.Table('ecg_dev_v1')

# initialize the dash app
app = Dash(__name__)

# define app layout
app.layout = html.Div([
    html.H2('ECG Livefeed'),
    html.Div(id='live-update-graph'),
    dcc.Graph(id='live-update-waveform'),
    dcc.Graph(id='live-update-waveform2'),
    dcc.Interval(
        id='interval-component',
        interval = 5000, # ms
        n_intervals=0
    )
])



# callback for auto-updates every interval (ms), s defined in layout
@callback(Output('live-update-waveform', 'figure'),
          Input('interval-component', 'n_intervals'))
def update_waveform(n):

    # get the timeframes to scan data
    ts_now = int(time.time()) * 1000
    prev_ts = int(ts_now - 200000)
    
    # scan dynamoDB table and extract items
    date_filter = Key('insert_time').between(prev_ts, ts_now)
    ecg_data = table.scan(FilterExpression=date_filter) # 
    items = ecg_data['Items']

    # create a datafranme and convert to correct data types
    # df = pd.DataFrame(items)
    # df['measurement'] = pd.to_numeric(df['measurement'])
    # df['measurement_id'] = pd.to_numeric(df['measurement_id'], downcast='unsigned')
    # df['measurement_time'] = pd.to_numeric(df['measurement_time'], downcast='unsigned')

    # Your DynamoDB response
    dynamo_response =items

    # Function to recursively convert Decimal to float
    def convert_decimal_to_float(item):
        if isinstance(item, Decimal):
            return float(item)
        elif isinstance(item, dict):
            return {key: convert_decimal_to_float(value) for key, value in item.items()}
        elif isinstance(item, list):
            return [convert_decimal_to_float(element) for element in item]
        else:
            return item

    # Convert Decimal to float in the response
    dynamo_response_float = convert_decimal_to_float(dynamo_response)

    # Create a pandas DataFrame
    df = pd.json_normalize(dynamo_response_float)

    # Ensure all values are numeric
    df = df.apply(pd.to_numeric, errors='coerce')

    df['insert_time'] = pd.to_datetime(df['insert_time'], unit='ms')
    # Sort DataFrame by 'measure_id'
    df = df.sort_values(by='device_data.measurement_id')

    fig = px.line(df, x= 'insert_time', y='device_data.measurement',  
                  labels={'measure_id': 'Measurement Time', 'measurement': 'Measurement'},
                  title='DynamoDB Data Visualization')
    
    return fig



# callback for the second graph
@callback(Output('live-update-waveform2', 'figure'),
          Input('interval-component', 'n_intervals'))
def update_second_graph(n):
    # get the timeframes to scan data
    ts_now = int(time.time()) * 1000
    prev_ts = int(ts_now - 200000)
    
    # scan dynamoDB table and extract items
    date_filter = Key('insert_time').between(prev_ts, ts_now)
    ecg_data = table.scan(FilterExpression=date_filter) # 
    items = ecg_data['Items']

    # Your DynamoDB response
    dynamo_response =items

    # Function to recursively convert Decimal to float
    def convert_decimal_to_float(item):
        if isinstance(item, Decimal):
            return float(item)
        elif isinstance(item, dict):
            return {key: convert_decimal_to_float(value) for key, value in item.items()}
        elif isinstance(item, list):
            return [convert_decimal_to_float(element) for element in item]
        else:
            return item

    # Convert Decimal to float in the response
    dynamo_response_float = convert_decimal_to_float(dynamo_response)

    # Create a pandas DataFrame
    df = pd.json_normalize(dynamo_response_float)

    # Ensure all values are numeric
    df = df.apply(pd.to_numeric, errors='coerce')

    df['insert_time'] = pd.to_datetime(df['insert_time'], unit='ms')
    # Sort DataFrame by 'measure_id'
    df = df.sort_values(by='device_data.measurement_id')

    fig = px.line(df, x= 'insert_time', y='device_data.measurement',  
                  labels={'measure_id': 'Measurement Time', 'measurement': 'Measurement'},
                  title='DynamoDB Data Visualization')
    
    return fig

if __name__ == '__main__':
    app.run_server(debug=True)

import dash
from dash import Dash, dcc, html, callback, Input, Output

import boto3
from boto3.dynamodb.conditions import Key, Attr
import pandas as pd
import plotly.express as px
import time

# define the AWS DynamoDB table object
ecg_dev_tab = boto3.resource('dynamodb', region_name='eu-west-1')
table = ecg_dev_tab.Table('ecg_dev')

# initialize the dash app
app = Dash(__name__)

# define app layout
app.layout = html.Div([
    html.H2('ECG Livefeed'),
    html.Div(id='live-update-graph'),
    dcc.Graph(id='live-update-waveform'),
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
    prev_ts = int(ts_now - 20000)
    
    # scan dynamoDB table and extract items
    date_filter = Key('measurement_time').between(prev_ts, ts_now)
    ecg_data = table.scan(FilterExpression=date_filter)
    items = ecg_data['Items']

    # create a datafranme and convert to correct data types
    df = pd.DataFrame(items)
    df['measurement'] = pd.to_numeric(df['measurement'])
    df['measurement_id'] = pd.to_numeric(df['measurement_id'], downcast='unsigned')
    df['measurement_time'] = pd.to_numeric(df['measurement_time'], downcast='unsigned')
    df['measurement_time'] = pd.to_datetime(df['measurement_time'], unit='ms')
    # Sort DataFrame by 'measure_id'
    df = df.sort_values(by='measurement_id')

    fig = px.line(df, x= 'measurement_time', y='measurement',  
                  labels={'measure_id': 'Measurement Time', 'measurement': 'Measurement'},
                  title='DynamoDB Data Visualization')
    
    return fig

if __name__ == '__main__':
    app.run_server(debug=True)

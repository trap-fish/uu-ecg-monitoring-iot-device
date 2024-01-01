import dash
from dash import dcc, html
from dash.dependencies import Input, Output
import boto3
import pandas as pd
import plotly.express as px

# Initialize Dash app
app = dash.Dash(__name__)

# AWS DynamoDB Configuration
dynamodb = boto3.resource('dynamodb', region_name='region-region-1')
table_name = '<table_name'
table = dynamodb.Table(table_name)

# Layout of the app
app.layout = html.Div([
    html.H1("DynamoDB Graph"),
    dcc.Graph(id='graph'),
    dcc.Interval(
        id='interval-component',
        interval=1 * 1000,  # in milliseconds
        n_intervals=0
    ),
])

# Callback to update the graph
@app.callback(Output('graph', 'figure'), [Input('interval-component', 'n_intervals')])
def update_graph(n):
    # Query data from DynamoDB
    response = table.scan()
    items = response['Items']

    # Convert DynamoDB items to DataFrame
    df = pd.DataFrame(items)

    # Convert 'measurement' column to numeric type and handle NaN values
    df['measurement'] = pd.to_numeric(df['measurement'], errors='coerce')

    # Drop NaN values for 'measurement'
    df = df.dropna(subset=['measurement'])

    # Create a line graph using Plotly Express
    fig = px.line(df, x='measurement_time', y='measurement', labels={'measurement_time': 'Measurement Time', 'measurement': 'Measurement'},
                  title='DynamoDB Data Visualization')

    return fig

if __name__ == '__main__':
    app.run_server(debug=True)

import QtQuick
import QtQuick.Controls

CounterChartFromTo
{
	id: timelineCounter
	title: "Timeline Stats"
	
	fromPropertyName: "TimeFrom"
	toPropertyName: "TimeTo"
	minPropertyName: "MinTime"
	maxPropertyName: "MaxTime"
	
	selectedValueFrom: function(pValue)
	{
		return pValue;
	}
	
	selectedValueTo: function(pValue)
	{
		var locale = Qt.locale();
		
		var dateTime = Date.fromLocaleString(locale, pValue, "yyyy/MM/dd");
		var toDate = new Date(dateTime)
		toDate.setDate(dateTime.getDate() + Math.ceil(timeLineStep.value));

		return toDate.toLocaleDateString(locale, "yyyy/MM/dd");
	}
	
	SpinBox
	{
		id: timeLineStep
		x: 100
		y: 20
		value: MainQmlBinder.TimelineStep / (24 * 3600)
		from: 1
		to: 365
		stepSize: 1
		editable: true
		background: Rectangle
		{
			color: "white"
			border.color: "gray"
			border.width: 2
			radius: 10
		}
		
		onValueChanged:
		{
			MainQmlBinder.TimelineStep = value*24*3600;
		}
	}
}
import QtQuick
import QtQuick.Controls

TextField 
{
    id: root

    signal dateAccepted(string standardizedDate)
    signal dateEditionFinished(string standardizedDate)

    onAccepted: 
    {
        var lFormatted = parseAndFormatDate(text)
        if (lFormatted !== "") 
        {
            text = lFormatted
            root.dateAccepted(lFormatted)
        } 
        else 
        {
            root.dateAccepted("")
        }
    }
	
	onEditingFinished: 
    {
        var lFormatted = parseAndFormatDate(text)
        if (lFormatted !== "") 
        {
            text = lFormatted
            root.dateEditionFinished(lFormatted)
        } 
        else 
        {
            root.dateEditionFinished("")
        }
    }

    function parseAndFormatDate(pInput) 
    {
        var lParts = pInput.trim().split(/[\/\-\.\s]+/).filter(function(pPart) 
        { 
            return pPart.length > 0 
        })

        if (lParts.length !== 3) 
        {
            return ""
        }

        var lYear, lMonth, lDay
        var lP0 = parseInt(lParts[0], 10)
        var lP2 = parseInt(lParts[2], 10)

        if (lParts[0].length === 4 || lP0 > 31) 
        {
            lYear = lParts[0]
            lMonth = lParts[1]
            lDay = lParts[2]
        } 
        else if (lParts[2].length === 4 || lP2 > 31) 
        {
            lDay = lParts[0]
            lMonth = lParts[1]
            lYear = lParts[2]
        } 
        else 
        {
            lDay = lParts[0]
            lMonth = lParts[1]
            lYear = lParts[2]
            if (lYear.length === 2) 
            {
                lYear = "20" + lYear
            }
        }

        var lYNum = parseInt(lYear, 10)
        var lMNum = parseInt(lMonth, 10)
        var lDNum = parseInt(lDay, 10)

        if (isNaN(lYNum) || isNaN(lMNum) || isNaN(lDNum)) 
        {
            return ""
        }

        if (lMNum < 1 || lMNum > 12 || lDNum < 1 || lDNum > 31) 
        {
            return ""
        }

        var lSYear = String(lYNum).padStart(4, '0')
        var lSMonth = String(lMNum).padStart(2, '0')
        var lSDay = String(lDNum).padStart(2, '0')

        return lSYear + "/" + lSMonth + "/" + lSDay
    }
}

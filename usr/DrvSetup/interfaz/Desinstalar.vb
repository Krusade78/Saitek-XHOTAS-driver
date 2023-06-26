Public Class Desinstalar

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
        Me.ParentForm.Close()
    End Sub

    Private Sub Desinstalar_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Select Case (System.Globalization.CultureInfo.CurrentCulture.LCID And &HFF)
            Case &HC
                'nombre = "french.xml"
            Case &H10
                'nombre = "italian.xml"
            Case &HA
                Label1.Text = "Desinstalar controladores XHOTAS"
                Label2.Text = "Se encontraron errores"
                Button1.Text = "Finalizar"
            Case &H7
                'nombre = "german.xml"
        End Select
    End Sub
End Class
